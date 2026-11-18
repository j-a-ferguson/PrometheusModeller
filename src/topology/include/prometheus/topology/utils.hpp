#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <armadillo>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "prometheus/common.hpp"
#include "types.hpp"

namespace prometheus::topology {

class IndexResource {

  public:
    IndexResource();
    NodeId peek_id() const;
    NodeId acquire_id();
    void release_id(NodeId);

  private:
    NodeId next_id_;
    std::set<NodeId> free_ids_;
};

using Index = std::size_t;
using Generation = std::uint32_t;

template <typename T> class SlotPool;

template <typename T> class Handle {

    public: 

    Handle(SlotPool<T>& pool, Index index, Generation gen): 
        pool_{&pool}, index_{index}, gen_{gen} {

        }
    
    T& get() {
        auto opt_val = pool_->get(*this);
        if(!opt_val) {
            throw std::logic_error("Handle is stale");
        }
        return opt_val.value();
    }

    const T& get() const {
        auto opt_val = pool_->get(*this);
        if(!opt_val) {
            throw std::logic_error("Handle is stale");
        }
        return opt_val.value();
    }

    const Index& index() const {
        return index_;
    }

    const Generation& gen() const {
        return gen_;
    }

    private:
    friend class SlotPool<T>;

    SlotPool<T> *pool_;
    Index index_;
    Generation gen_;
};

template <typename T> class SlotPool {

    friend class Handle<T>;

  private:
    struct Slot {
        // next is active when slot is free, value is active when live
        union {
            Index next;
            T value;
        };
        // generation number, even is free, odd is live
        Generation gen{0};

        bool is_live() const { return gen & 1; }

        Slot() : next{NULL_ID} {}
        // vector growth moves slots, so a Slot must move whichever member is active.
        Slot(Slot &&o) noexcept(std::is_nothrow_move_constructible_v<T>) : gen(o.gen) {
            if (is_live())
                std::construct_at(&value, std::move(o.value));
            else
                next = o.next;
        }
        ~Slot() {
            if (is_live())
                std::destroy_at(&value);
        }
        Slot(const Slot &) = delete;
        Slot &operator=(const Slot &) = delete;
        Slot &operator=(Slot &&) = delete;
    };

    template <bool Const> class Iter {
      private:
        using SlotPtr = std::conditional_t<Const, const Slot *, Slot *>;
        using SlotPoolPtr = std::conditional_t<Const, const SlotPool<T> *, SlotPool<T> *>;

        SlotPoolPtr pool_{nullptr};
        SlotPtr start_{nullptr};
        SlotPtr cur_{nullptr};
        SlotPtr end_{nullptr};

        void skip_free() {
            while (cur_ != end_ && !cur_->is_live())
                ++cur_;
        }

      public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = std::conditional_t<Const, const T &, T &>;
        using pointer = std::conditional_t<Const, const T *, T *>;

        Iter() = default;
        Iter(SlotPoolPtr pool, SlotPtr start, SlotPtr cur, SlotPtr end)
            : pool_{pool}, start_{start}, cur_{cur}, end_{end} {
            skip_free();
        }

        operator Iter<true>() const
            requires(!Const)
        {
            return {pool_, start_, cur_, end_};
        }

        reference operator*() const { return cur_->value; }

        pointer operator->() const { return &cur_->value; }

        Iter &operator++() {
            ++cur_;
            skip_free();
            return *this;
        }

        Iter operator++(int) {
            Iter out = *this;
            ++cur_;
            skip_free();
            return out;
        }

        friend bool operator==(const Iter &a, const Iter &b) { return a.cur_ == b.cur_; }

        Handle<T> handle() const requires (!Const) {
            return Handle<T>{*pool_, static_cast<Index>(cur_ - start_), cur_->gen};
        }
    };

    std::vector<Slot> slots_;
    Index head_ = NULL_ID;

    Slot *find(Handle<T> h) {
        require_same_pool(h);
        if (h.index_ >= slots_.size())
            return nullptr;
        Slot &slot = slots_[h.index_];
        return h.gen_ == slot.gen && slot.is_live() ? &slot : nullptr;
    }

    const Slot *find(Handle<T> h) const {
        require_same_pool(h);
        if (h.index_ >= slots_.size())
            return nullptr;
        const Slot &slot = slots_[h.index_];
        return h.gen_ == slot.gen && slot.is_live() ? &slot : nullptr;
    }

    Index acquire_index() {
        if (is_null(head_)) {
            // free list empty, add new element
            slots_.emplace_back();
            return slots_.size() - 1;
        }
        Index idx = head_;
        head_ = slots_[idx].next;
        return idx;
    }

    void release_index(Index i) {
        slots_[i].next = head_;
        head_ = i;
    }

  public:
    using iterator = Iter<false>;
    using const_iterator = Iter<true>;

    SlotPool() {}

    static SlotPool<T> with_capacity(size_t capacity) {
        SlotPool<T> out;
        out.slots_.reserve(capacity);
        return out;
    }

    iterator begin() {
        return iterator(this, slots_.data(), slots_.data(), slots_.data() + slots_.size());
    }
    iterator end() {
        return iterator{this, slots_.data(), slots_.data() + slots_.size(),
                        slots_.data() + slots_.size()};
    }
    const_iterator cbegin() const {
        return const_iterator(this, slots_.data(), slots_.data(), slots_.data() + slots_.size());
    }
    const_iterator cend() const {
        return const_iterator{this, slots_.data(), slots_.data() + slots_.size(),
                              slots_.data() + slots_.size()};
    }

    template <typename... Args> Handle<T> emplace(Args &&...args) {
        const auto idx = acquire_index();
        Slot &s = slots_[idx];
        try {
            std::construct_at(&s.value, std::forward<Args>(args)...);
        } catch (...) {
            release_index(idx);
            throw;
        }
        ++s.gen;
        return Handle<T>{*this, idx, s.gen};
    }

    bool erase(Handle<T> handle) {
        Slot *s = find(handle);
        if (s == nullptr)
            return false;

        std::destroy_at(&(s->value));
        ++s->gen;
        release_index(handle.index_);
        return true;
    }

    OptRef<T> get(Handle<T> handle) {
        Slot *slot = find(handle);
        if (slot == nullptr)
            return std::nullopt;
        return std::ref(slot->value);
    }

    ConstOptRef<T> get(Handle<T> handle) const {
        const Slot *slot = find(handle);
        if (slot == nullptr)
            return std::nullopt;
        return std::cref(slot->value);
    }

    bool is_stale(Handle<T> handle) const {
        const Slot *slot = find(handle);
        return slot == nullptr;
    }

    void require_same_pool(Handle<T> handle) const {
        if (handle.pool_ != this) {
            throw std::logic_error("Handle from wrong slot pool");
        }
    }
};
} // namespace prometheus::topology

#endif
