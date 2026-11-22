#ifndef INPLACE_VECTOR_HPP_
#define INPLACE_VECTOR_HPP_

#include <algorithm>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace prometheus::containers {
namespace detail {

template <class T, std::size_t N> struct InplaceBuffer {
    // The byte array provides storage for an implicitly created T[N] array.
    // Individual elements are explicitly constructed as the vector grows.
    alignas(T) std::byte bytes[sizeof(T) * N];

    T *data() noexcept { return *std::launder(reinterpret_cast<T(*)[N]>(bytes)); }
    const T *data() const noexcept { return *std::launder(reinterpret_cast<const T(*)[N]>(bytes)); }
};

// Trivial elements can use a typed array even during C++23 constant evaluation.
// The union's defaulted copy operations copy its representation, including unused slots.
template <class T, std::size_t N>
    requires(std::is_trivial_v<T> && std::is_default_constructible_v<T>)
struct InplaceBuffer<T, N> {
    struct Array {
        T elements[N];
        constexpr Array() noexcept {}
    };
    union {
        Array array;
    };
    constexpr InplaceBuffer() noexcept : array{} {}
    constexpr T *data() noexcept { return array.elements; }
    constexpr const T *data() const noexcept { return array.elements; }
};

template <class T, std::size_t N> struct InplaceStorage {
    InplaceBuffer<T, N> buffer;
    std::size_t count = 0;

    constexpr T *data() noexcept { return buffer.data(); }
    constexpr const T *data() const noexcept { return buffer.data(); }
    constexpr std::size_t size() const noexcept { return count; }
    constexpr void set_size(std::size_t n) noexcept { count = n; }
};

template <class T> struct InplaceStorage<T, 0> {
    constexpr T *data() noexcept { return nullptr; }
    constexpr const T *data() const noexcept { return nullptr; }
    constexpr std::size_t size() const noexcept { return 0; }
    constexpr void set_size(std::size_t) noexcept {}
};

template <class R, class T>
concept InplaceInputRange =
    std::ranges::input_range<R> && std::constructible_from<T, std::ranges::range_reference_t<R>>;

} // namespace detail

template <typename T, std::size_t N> class InplaceVector : private detail::InplaceStorage<T, N> {
    using Storage = detail::InplaceStorage<T, N>;
    static constexpr bool trivial_storage =
        N == 0 || (std::is_trivial_v<T> && std::is_default_constructible_v<T>);

  public:
    using value_type = T;
    using value = T;
    using const_value = const T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using Storage::data;
    using Storage::size;

    constexpr InplaceVector() noexcept = default;
    constexpr explicit InplaceVector(size_type count) : InplaceVector() { resize(count); }
    constexpr InplaceVector(size_type count, const T &value) : InplaceVector() {
        resize(count, value);
    }

    template <std::input_iterator I, std::sentinel_for<I> S>
    constexpr InplaceVector(I first, S last) : InplaceVector() {
        append_range(std::ranges::subrange(std::move(first), std::move(last)));
    }

    template <detail::InplaceInputRange<T> R>
    constexpr InplaceVector(std::from_range_t, R &&range) : InplaceVector() {
        append_range(std::forward<R>(range));
    }

    constexpr InplaceVector(std::initializer_list<T> values)
        : InplaceVector(values.begin(), values.end()) {}

    constexpr InplaceVector(const InplaceVector &)
        requires trivial_storage
    = default;
    constexpr InplaceVector(const InplaceVector &other)
        requires(!trivial_storage && std::is_copy_constructible_v<T>)
        : InplaceVector(other.begin(), other.end()) {}

    constexpr InplaceVector(InplaceVector &&)
        requires trivial_storage
    = default;
    constexpr InplaceVector(InplaceVector &&other) noexcept(std::is_nothrow_move_constructible_v<T>)
        requires(!trivial_storage && std::is_move_constructible_v<T>)
        : InplaceVector(std::make_move_iterator(other.begin()),
                        std::make_move_iterator(other.end())) {}

    constexpr ~InplaceVector()
        requires(N == 0 || std::is_trivially_destructible_v<T>)
    = default;
    constexpr ~InplaceVector() { clear(); }

    constexpr InplaceVector &operator=(const InplaceVector &)
        requires trivial_storage
    = default;
    constexpr InplaceVector &operator=(const InplaceVector &other)
        requires(!trivial_storage && std::is_copy_constructible_v<T> &&
                 std::is_copy_assignable_v<T>)
    {
        if (this != &other) {
            assign(other.begin(), other.end());
        }
        return *this;
    }
    constexpr InplaceVector &operator=(InplaceVector &&)
        requires trivial_storage
    = default;
    constexpr InplaceVector &
    operator=(InplaceVector &&other) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                              std::is_nothrow_move_assignable_v<T>)
        requires(!trivial_storage && std::is_move_constructible_v<T> &&
                 std::is_move_assignable_v<T>)
    {
        if (this != &other) {
            assign(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
        }
        return *this;
    }
    constexpr InplaceVector &operator=(std::initializer_list<T> values) {
        assign(values);
        return *this;
    }

    constexpr void assign(size_type count, const T &value) {
        reserve(count);
        std::fill_n(begin(), std::min(size(), count), value);
        resize(count, value);
    }

    template <std::input_iterator I, std::sentinel_for<I> S>
    constexpr void assign(I first, S last) {
        if constexpr (std::sized_sentinel_for<S, I> || std::forward_iterator<I>) {
            reserve(static_cast<size_type>(std::ranges::distance(first, last)));
        }
        auto dest = begin();
        while (dest != end() && first != last) {
            *dest++ = *first;
            ++first;
        }
        truncate(static_cast<size_type>(dest - begin()));
        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

    template <detail::InplaceInputRange<T> R> constexpr void assign_range(R &&range) {
        assign(std::ranges::begin(range), std::ranges::end(range));
    }
    constexpr void assign(std::initializer_list<T> values) { assign_range(values); }

    constexpr iterator begin() noexcept { return data(); }
    constexpr const_iterator begin() const noexcept { return data(); }
    constexpr iterator end() noexcept { return begin() + size(); }
    constexpr const_iterator end() const noexcept { return begin() + size(); }
    constexpr const_iterator cbegin() const noexcept { return begin(); }
    constexpr const_iterator cend() const noexcept { return end(); }
    constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }
    constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }
    constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    constexpr const_reverse_iterator crend() const noexcept { return rend(); }

    constexpr bool empty() const noexcept { return size() == 0; }
    static constexpr size_type capacity() noexcept { return N; }
    static constexpr size_type max_size() noexcept { return N; }
    static constexpr void reserve(size_type count) {
        if (count > N) {
            throw std::bad_alloc();
        }
    }
    static constexpr void shrink_to_fit() noexcept {}
    constexpr void resize(size_type count) { resize_to(count); }
    constexpr void resize(size_type count, const T &value) { resize_to(count, value); }

    constexpr reference operator[](size_type i) noexcept { return data()[i]; }
    constexpr const_reference operator[](size_type i) const noexcept { return data()[i]; }
    constexpr reference at(size_type i) {
        return const_cast<reference>(std::as_const(*this).at(i));
    }
    constexpr const_reference at(size_type i) const {
        if (i >= size()) {
            throw std::out_of_range("InplaceVector index out of range");
        }
        return (*this)[i];
    }
    constexpr reference front() { return at(0); }
    constexpr const_reference front() const { return at(0); }
    constexpr reference back() { return at(size() - 1); }
    constexpr const_reference back() const { return at(size() - 1); }

    template <class... Args> constexpr reference emplace_back(Args &&...args) {
        if (auto element = try_emplace_back(std::forward<Args>(args)...)) {
            return *element;
        }
        throw std::bad_alloc();
    }
    constexpr reference push_back(const T &value) { return emplace_back(value); }
    constexpr reference push_back(T &&value) { return emplace_back(std::move(value)); }

    template <class... Args> constexpr pointer try_emplace_back(Args &&...args) {
        if (size() == N) {
            return nullptr;
        }
        return std::addressof(unchecked_emplace_back(std::forward<Args>(args)...));
    }
    constexpr pointer try_push_back(const T &value) { return try_emplace_back(value); }
    constexpr pointer try_push_back(T &&value) { return try_emplace_back(std::move(value)); }

    template <class... Args> constexpr reference unchecked_emplace_back(Args &&...args) {
        assert(size() < N);
        auto element = std::construct_at(end(), std::forward<Args>(args)...);
        this->set_size(size() + 1);
        return *element;
    }
    constexpr reference unchecked_push_back(const T &value) {
        return unchecked_emplace_back(value);
    }
    constexpr reference unchecked_push_back(T &&value) {
        return unchecked_emplace_back(std::move(value));
    }

    template <detail::InplaceInputRange<T> R>
    constexpr std::ranges::borrowed_iterator_t<R> try_append_range(R &&range) {
        return append_until_full(std::ranges::begin(range), std::ranges::end(range));
    }

    template <detail::InplaceInputRange<T> R> constexpr void append_range(R &&range) {
        if constexpr (std::ranges::sized_range<R>) {
            if (std::ranges::size(range) > N - size()) {
                throw std::bad_alloc();
            }
        }
        auto old_size = size();
        try {
            auto last = std::ranges::end(range);
            if (append_until_full(std::ranges::begin(range), last) != last) {
                throw std::bad_alloc();
            }
        } catch (...) {
            truncate(old_size);
            throw;
        }
    }

    template <class... Args> constexpr iterator emplace(const_iterator position, Args &&...args) {
        auto dest = begin() + (position - cbegin());
        auto old_end = end();
        emplace_back(std::forward<Args>(args)...);
        std::rotate(dest, old_end, end());
        return dest;
    }
    constexpr iterator insert(const_iterator pos, const T &value) { return emplace(pos, value); }
    constexpr iterator insert(const_iterator pos, T &&value) {
        return emplace(pos, std::move(value));
    }
    constexpr iterator insert(const_iterator pos, size_type count, const T &value) {
        if (count > N - size()) {
            throw std::bad_alloc();
        }
        auto dest = begin() + (pos - cbegin());
        auto old_end = end();
        resize(size() + count, value);
        std::rotate(dest, old_end, end());
        return dest;
    }
    template <std::input_iterator I, std::sentinel_for<I> S>
    constexpr iterator insert(const_iterator pos, I first, S last) {
        return insert_range(pos, std::ranges::subrange(std::move(first), std::move(last)));
    }
    template <detail::InplaceInputRange<T> R>
    constexpr iterator insert_range(const_iterator pos, R &&range) {
        auto dest = begin() + (pos - cbegin());
        auto old_end = end();
        append_range(std::forward<R>(range));
        std::rotate(dest, old_end, end());
        return dest;
    }
    constexpr iterator insert(const_iterator pos, std::initializer_list<T> values) {
        return insert_range(pos, values);
    }

    constexpr iterator erase(const_iterator pos) { return erase(pos, pos + 1); }
    constexpr iterator erase(const_iterator first, const_iterator last) {
        auto dest = begin() + (first - cbegin());
        if (first != last) {
            auto new_end = std::move(begin() + (last - cbegin()), end(), dest);
            truncate(static_cast<size_type>(new_end - begin()));
        }
        return dest;
    }
    constexpr void pop_back() {
        if (empty()) {
            throw std::out_of_range("InplaceVector is empty");
        }
        truncate(size() - 1);
    }
    constexpr void clear() noexcept { truncate(0); }

    constexpr void swap(InplaceVector &other) noexcept(N == 0 ||
                                                       (std::is_nothrow_swappable_v<T> &&
                                                        std::is_nothrow_move_constructible_v<T>)) {
        if constexpr (N != 0) {
            if (this == &other) {
                return;
            }
            if (size() > other.size()) {
                other.swap(*this);
                return;
            }
            auto common = size();
            std::swap_ranges(begin(), end(), other.begin());
            for (auto i = common; i < other.size(); ++i) {
                unchecked_emplace_back(std::move(other[i]));
            }
            other.truncate(common);
        }
    }
    friend constexpr void swap(InplaceVector &a, InplaceVector &b) noexcept(noexcept(a.swap(b))) {
        a.swap(b);
    }
    friend constexpr bool operator==(const InplaceVector &a, const InplaceVector &b) {
        return std::equal(a.begin(), a.end(), b.begin(), b.end());
    }
    friend constexpr auto operator<=>(const InplaceVector &a, const InplaceVector &b)
        requires requires(const T &x, const T &y) {
            { x < y } -> std::convertible_to<bool>;
        }
    {
        auto compare = [](const T &x, const T &y) {
            if constexpr (std::three_way_comparable<T>) {
                return x <=> y;
            } else {
                if (x < y)
                    return std::weak_ordering::less;
                if (y < x)
                    return std::weak_ordering::greater;
                return std::weak_ordering::equivalent;
            }
        };
        return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end(),
                                                      compare);
    }

  private:
    template <class I, class S> constexpr I append_until_full(I first, const S &last) {
        while (size() < N && first != last) {
            unchecked_emplace_back(*first);
            ++first;
        }
        return first;
    }

    constexpr void truncate(size_type count) noexcept {
        while (size() > count) {
            this->set_size(size() - 1);
            std::destroy_at(end());
        }
    }
    template <class... Args> constexpr void resize_to(size_type count, const Args &...args) {
        reserve(count);
        auto old_size = size();
        truncate(count);
        try {
            while (size() < count) {
                unchecked_emplace_back(args...);
            }
        } catch (...) {
            truncate(old_size);
            throw;
        }
    }
};

template <class T, std::size_t N, class U = T>
constexpr std::size_t erase(InplaceVector<T, N> &v, const U &value) {
    auto first = std::remove(v.begin(), v.end(), value);
    auto removed = static_cast<std::size_t>(v.end() - first);
    v.erase(first, v.end());
    return removed;
}

template <class T, std::size_t N, class Pred>
constexpr std::size_t erase_if(InplaceVector<T, N> &v, Pred pred) {
    auto first = std::remove_if(v.begin(), v.end(), std::move(pred));
    auto removed = static_cast<std::size_t>(v.end() - first);
    v.erase(first, v.end());
    return removed;
}

} // namespace prometheus::containers

#endif
