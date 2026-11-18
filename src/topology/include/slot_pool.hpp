#pragma once

#include "common.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace prometheus::topology {

using Index = std::size_t;
using Generation = std::uint32_t;

template <typename T> class SlotPool;

template <typename T> class Handle {

  public:
    Handle(SlotPool<T> &pool, Index index, Generation gen);
    T &get();
    const T &get() const;
    const Index &index() const;
    const Generation &gen() const;

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
} // namespace prometheus::topology
