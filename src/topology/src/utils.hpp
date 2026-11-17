#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <types.hpp>

#include <set>
#include <vector>

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

struct Handle 
{
    Index index;
    Generation gen;
};

template <typename T> class SlotPool {

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

    template <bool Const>
    class Iter {
        private: 

        using SlotPtr = std::conditional_t<Const, const Slot*, Slot*>;

        SlotPtr start_{nullptr};
        SlotPtr cur_{nullptr};
        SlotPtr end_{nullptr};

        void skip_free() {
            while (cur_!= end_ && !cur_->is_live()) ++cur_;
        }


        public: 

        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = std::conditional_t<Const, const T&, T&>;
        using pointer = std::conditional_t<Const, const T*, T*>;

        Iter() = default;
        Iter(SlotPtr start, SlotPtr cur, SlotPtr end): start_{start}, cur_{cur}, end_{end_} {
            skip_free();
        }

        operator Iter<true>() const requires (!Const) { return {start_, cur_, end_}; }

        reference operator*() const {
            return cur_->value;
        }

        pointer operator->() const {
            return &cur_->value();
        }

        Iter& operator++() {
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

        friend bool operator==(const Iter& a, const Iter& b) {return a.cur_ == b.cur_;}

        Handle handle() const {
            return Handle{.index = static_cast<Index>(cur_ - start_), .gen = cur_->gen};
        }


    };

    std::vector<Slot> slots_;
    Index head_ = NULL_ID;

    public: 

    SlotPool(){}

    static SlotPool<T> with_capacity(size_t capacity) {
        SlotPool<T> out;
        out.slots_.reserve(capacity);
        return out;
    }




};
} // namespace prometheus::topology

#endif
