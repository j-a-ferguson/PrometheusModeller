#pragma once

#include "common.hpp"
#include "json_formatter.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace prometheus::topology {

using Index = std::size_t;
constexpr Index NULL_INDEX = std::numeric_limits<Index>::max();
using Generation = std::uint32_t;
constexpr Generation NULL_GEN =  std::numeric_limits<Generation>::max();

template <typename T> class SlotPool;

template <typename T> class Handle {

  public:
    Handle() = default;
    Handle(SlotPool<T> &pool, Index index, Generation gen);
    bool operator==(const Handle<T>& other) const = default;

    static Handle<T> null() {
      Handle<T> null_handle;
      return null_handle;
    };

    T &get();
    const T &get() const;
    const Index &index() const;
    const Generation &gen() const;
    void nullify();
    bool is_null() const;
    bool is_stale() const;

  private:
    friend class SlotPool<T>;

    SlotPool<T> *pool_{nullptr};
    Index index_{NULL_INDEX};
    Generation gen_{NULL_GEN};
};

template <typename T> void to_json(nlohmann::json &json, const Handle<T> &handle) {
    if (handle.is_null()) {
        json = nullptr;
        return;
    }
    json = {{"index", handle.index()}, {"generation", handle.gen()}};
}

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

        bool is_live() const;
        Slot();
        Slot(Slot &&other) noexcept(std::is_nothrow_move_constructible_v<T>);
        ~Slot();
        Slot(const Slot &) = delete;
        Slot &operator=(const Slot &) = delete;
        Slot &operator=(Slot &&) = delete;
    };

    template <bool Const> class Iter {
        using SlotPtr = std::conditional_t<Const, const Slot *, Slot *>;
        using SlotPoolPtr = std::conditional_t<Const, const SlotPool *, SlotPool *>;

        SlotPoolPtr pool_{nullptr};
        SlotPtr start_{nullptr};
        SlotPtr cur_{nullptr};
        SlotPtr end_{nullptr};
        void skip_free();

      public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = std::conditional_t<Const, const T &, T &>;
        using pointer = std::conditional_t<Const, const T *, T *>;

        Iter() = default;
        Iter(SlotPoolPtr pool, SlotPtr start, SlotPtr cur, SlotPtr end);
        operator Iter<true>() const requires (!Const);
        reference operator*() const;
        pointer operator->() const;
        Iter &operator++();
        Iter operator++(int);
        bool operator==(const Iter &other) const;
        Handle<T> handle() const requires (!Const);
    };

    std::vector<Slot> slots_;
    static constexpr Index no_slot = std::numeric_limits<Index>::max();
    Index head_ = no_slot;

    Slot *find(Handle<T> h);
    const Slot *find(Handle<T> h) const;
    Index acquire_index();
    void release_index(Index i);
    Handle<T> emplace_with(const std::function<void(T *)> &construct);

  public:
    using iterator = Iter<false>;
    using const_iterator = Iter<true>;

    SlotPool() = default;
    SlotPool(const SlotPool &) = delete;
    SlotPool &operator=(const SlotPool &) = delete;
    SlotPool(SlotPool &&) = default;
    SlotPool &operator=(SlotPool &&) = default;

    static SlotPool<T> with_capacity(std::size_t capacity);

    iterator begin();
    iterator end();
    const_iterator cbegin() const;
    const_iterator cend() const;

    template <typename... Args> Handle<T> emplace(Args &&...args) {
        return emplace_with([&](T *storage) {
            std::construct_at(storage, std::forward<Args>(args)...);
        });
    }

    bool erase(Handle<T> handle);
    OptRef<T> get(Handle<T> handle);
    ConstOptRef<T> get(Handle<T> handle) const;
    bool is_stale(Handle<T> handle) const;
    void require_same_pool(Handle<T> handle) const;
};

template <typename T>
Handle<T>::Handle(SlotPool<T> &pool, Index index, Generation gen)
    : pool_{&pool}, index_{index}, gen_{gen} {}

template <typename T> auto Handle<T>::get() -> T & {
    if (!pool_)
        throw std::logic_error("Handle is null");
    auto opt_val = pool_->get(*this);
    if (!opt_val) {
        throw std::logic_error("Handle is stale");
    }
    return opt_val->get();
}

template <typename T> auto Handle<T>::get() const -> const T & {
    if (!pool_)
        throw std::logic_error("Handle is null");
    auto opt_val = pool_->get(*this);
    if (!opt_val) {
        throw std::logic_error("Handle is stale");
    }
    return opt_val->get();
}

template <typename T> auto Handle<T>::index() const -> const Index & { return index_; }

template <typename T> auto Handle<T>::gen() const -> const Generation & { return gen_; }

template <typename T> void Handle<T>::nullify() {
    pool_ = nullptr;
    index_ = NULL_INDEX;
    gen_ = NULL_GEN;
}

template <typename T> bool Handle<T>::is_null() const {
    bool pool_null = pool_ == nullptr;
    bool index_null = index_ == NULL_INDEX;
    bool gen_null = gen_ == NULL_GEN;
    bool all_null = pool_null && index_null && gen_null;
    if (all_null)
        return true;
    bool all_not_null = !pool_null && !index_null && !gen_null;
    if (all_not_null)
        return false;
    throw std::logic_error("Corrupted internal state");
}

template <typename T> bool Handle<T>::is_stale() const {
    if (!pool_)
        throw std::logic_error("Handle is null");
    return !pool_->get(*this).has_value();
}

template <typename T> auto SlotPool<T>::Slot::is_live() const -> bool { return gen & 1; }

template <typename T> SlotPool<T>::Slot::Slot() : next{no_slot} {}

template <typename T>
SlotPool<T>::Slot::Slot(Slot &&o) noexcept(std::is_nothrow_move_constructible_v<T>) : gen(o.gen) {
    if (is_live())
        std::construct_at(&value, std::move(o.value));
    else
        next = o.next;
}

template <typename T> SlotPool<T>::Slot::~Slot() {
    if (is_live())
        std::destroy_at(&value);
}

template <typename T> template <bool Const> auto SlotPool<T>::Iter<Const>::skip_free() -> void {
    while (cur_ != end_ && !cur_->is_live())
        ++cur_;
}

template <typename T>
template <bool Const>
SlotPool<T>::Iter<Const>::Iter(SlotPoolPtr pool, SlotPtr start, SlotPtr cur, SlotPtr end)
    : pool_{pool}, start_{start}, cur_{cur}, end_{end} {
    skip_free();
}

template <typename T>
template <bool Const>
SlotPool<T>::Iter<Const>::operator Iter<true>() const
    requires(!Const)
{
    return {pool_, start_, cur_, end_};
}

template <typename T>
template <bool Const>
auto SlotPool<T>::Iter<Const>::operator*() const -> reference {
    return cur_->value;
}

template <typename T>
template <bool Const>
auto SlotPool<T>::Iter<Const>::operator->() const -> pointer {
    return &cur_->value;
}

template <typename T> template <bool Const> auto SlotPool<T>::Iter<Const>::operator++() -> Iter & {
    ++cur_;
    skip_free();
    return *this;
}

template <typename T> template <bool Const> auto SlotPool<T>::Iter<Const>::operator++(int) -> Iter {
    Iter out = *this;
    ++cur_;
    skip_free();
    return out;
}

template <typename T>
template <bool Const>
auto SlotPool<T>::Iter<Const>::operator==(const Iter &other) const -> bool {
    return cur_ == other.cur_;
}

template <typename T>
template <bool Const>
auto SlotPool<T>::Iter<Const>::handle() const -> Handle<T>
    requires(!Const)
{
    return Handle<T>{*pool_, static_cast<Index>(cur_ - start_), cur_->gen};
}

template <typename T> auto SlotPool<T>::find(Handle<T> h) -> Slot * {
    require_same_pool(h);
    if (h.index_ >= slots_.size())
        return nullptr;
    Slot &slot = slots_[h.index_];
    return h.gen_ == slot.gen && slot.is_live() ? &slot : nullptr;
}

template <typename T> auto SlotPool<T>::find(Handle<T> h) const -> const Slot * {
    require_same_pool(h);
    if (h.index_ >= slots_.size())
        return nullptr;
    const Slot &slot = slots_[h.index_];
    return h.gen_ == slot.gen && slot.is_live() ? &slot : nullptr;
}

template <typename T> auto SlotPool<T>::acquire_index() -> Index {
    if (head_ == no_slot) {
        // free list empty, add new element
        slots_.emplace_back();
        return slots_.size() - 1;
    }
    Index idx = head_;
    head_ = slots_[idx].next;
    return idx;
}

template <typename T> auto SlotPool<T>::release_index(Index i) -> void {
    slots_[i].next = head_;
    head_ = i;
}

template <typename T> auto SlotPool<T>::with_capacity(std::size_t capacity) -> SlotPool<T> {
    SlotPool<T> out;
    out.slots_.reserve(capacity);
    return out;
}

template <typename T> auto SlotPool<T>::begin() -> iterator {
    return iterator(this, slots_.data(), slots_.data(), slots_.data() + slots_.size());
}

template <typename T> auto SlotPool<T>::end() -> iterator {
    return iterator{this, slots_.data(), slots_.data() + slots_.size(),
                    slots_.data() + slots_.size()};
}

template <typename T> auto SlotPool<T>::cbegin() const -> const_iterator {
    return const_iterator(this, slots_.data(), slots_.data(), slots_.data() + slots_.size());
}

template <typename T> auto SlotPool<T>::cend() const -> const_iterator {
    return const_iterator{this, slots_.data(), slots_.data() + slots_.size(),
                          slots_.data() + slots_.size()};
}

template <typename T>
auto SlotPool<T>::emplace_with(const std::function<void(T *)> &construct) -> Handle<T> {
    const auto idx = acquire_index();
    Slot &s = slots_[idx];
    try {
        construct(&s.value);
    } catch (...) {
        release_index(idx);
        throw;
    }
    ++s.gen;
    return Handle<T>{*this, idx, s.gen};
}

template <typename T> auto SlotPool<T>::erase(Handle<T> handle) -> bool {
    Slot *s = find(handle);
    if (s == nullptr)
        return false;

    std::destroy_at(&(s->value));
    ++s->gen;
    release_index(handle.index_);
    return true;
}

template <typename T> auto SlotPool<T>::get(Handle<T> handle) -> OptRef<T> {
    Slot *slot = find(handle);
    if (slot == nullptr)
        return std::nullopt;
    return std::ref(slot->value);
}

template <typename T> auto SlotPool<T>::get(Handle<T> handle) const -> ConstOptRef<T> {
    const Slot *slot = find(handle);
    if (slot == nullptr)
        return std::nullopt;
    return std::cref(slot->value);
}

template <typename T> auto SlotPool<T>::is_stale(Handle<T> handle) const -> bool {
    const Slot *slot = find(handle);
    return slot == nullptr;
}

template <typename T> auto SlotPool<T>::require_same_pool(Handle<T> handle) const -> void {
    if (handle.pool_ != this) {
        throw std::logic_error("Handle from wrong slot pool");
    }
}

} // namespace prometheus::topology

template <typename T>
struct std::formatter<prometheus::topology::Handle<T>, char>
    : prometheus::JsonFormatter<prometheus::topology::Handle<T>> {};
