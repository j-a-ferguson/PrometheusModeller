#include "inplace_vector.hpp"

#include <gtest/gtest.h>

#include <array>
#include <compare>
#include <cstdint>
#include <limits>
#include <memory>
#include <ranges>
#include <sstream>
#include <string>
#include <type_traits>

namespace prometheus::containers {
namespace {

using Ints = InplaceVector<int, 8>;
using Owners = InplaceVector<std::unique_ptr<int>, 4>;
static_assert(std::ranges::contiguous_range<Ints>);
static_assert(std::ranges::contiguous_range<const Ints>);
static_assert(std::is_trivially_copyable_v<Ints>);
static_assert(!std::is_copy_constructible_v<Owners>);
static_assert(!std::is_copy_assignable_v<Owners>);
static_assert(std::is_nothrow_move_constructible_v<Owners>);
static_assert(std::is_nothrow_move_assignable_v<Owners>);
static_assert(std::is_empty_v<InplaceVector<std::string, 0>>);
static_assert(std::is_trivially_copyable_v<InplaceVector<std::string, 0>>);
static_assert(Ints::max_size() == 8 && Ints::capacity() == 8);
static_assert([] {
    Ints v{1, 3};
    v.insert(v.begin() + 1, 2);
    v.pop_back();
    return v == Ints{1, 2};
}());

// A non-default-constructible element with observable lifetimes and a throwing copy.
struct alignas(64) Tracked {
    int &alive;
    int &copies_left;
    int value;

    Tracked(int &alive, int &copies_left, int value)
        : alive(alive), copies_left(copies_left), value(value) {
        ++alive;
    }
    Tracked(const Tracked &other)
        : alive(other.alive), copies_left(other.copies_left), value(other.value) {
        if (copies_left == 0)
            throw std::runtime_error("copy failed");
        --copies_left;
        ++alive;
    }
    Tracked(Tracked &&other) noexcept
        : Tracked(other.alive, other.copies_left, std::exchange(other.value, -1)) {}
    Tracked &operator=(const Tracked &other) {
        value = other.value;
        return *this;
    }
    Tracked &operator=(Tracked &&other) noexcept {
        value = std::exchange(other.value, -1);
        return *this;
    }
    ~Tracked() { --alive; }
};

TEST(InplaceVector, ConstructorsAndAssignment) {
    EXPECT_TRUE(Ints().empty());
    EXPECT_EQ(Ints(3), (Ints{0, 0, 0}));
    EXPECT_EQ(Ints(3, 7), (Ints{7, 7, 7}));
    const std::array source{1, 2, 3};
    Ints v(source.begin(), source.end());
    EXPECT_EQ(v, Ints(std::from_range, source));
    auto copy = v;
    copy.front() = 9;
    EXPECT_EQ(v.front(), 1);
    v = copy;
    EXPECT_EQ(v, (Ints{9, 2, 3}));
    v = {8, 6};
    v.assign(source.begin(), source.end());
    EXPECT_EQ(v, (Ints{1, 2, 3}));
    v.assign(4, v.back());
    EXPECT_EQ(v, (Ints{3, 3, 3, 3}));
    v.assign({5, 6});
    EXPECT_EQ(v, (Ints{5, 6}));
    v.assign_range(source | std::views::reverse);
    EXPECT_EQ(v, (Ints{3, 2, 1}));
    EXPECT_THROW(Ints(9), std::bad_alloc);
    EXPECT_THROW(v.assign(9, 0), std::bad_alloc);
    EXPECT_EQ(v, (Ints{3, 2, 1}));
}

TEST(InplaceVector, ElementAccessAndIteratorsReferToInlineStorage) {
    Ints v{1, 2, 3};
    const auto &cv = v;
    auto first = v.data();
    auto address = reinterpret_cast<std::uintptr_t>(first);
    auto object = reinterpret_cast<std::uintptr_t>(&v);
    EXPECT_GE(address, object);
    EXPECT_LE(address + sizeof(int) * v.capacity(), object + sizeof(v));
    EXPECT_EQ(v.begin(), first);
    EXPECT_EQ(cv.data(), cv.begin());
    EXPECT_EQ(cv.cbegin(), cv.begin());
    EXPECT_EQ(cv.cend(), cv.end());
    EXPECT_EQ(cv.end() - cv.begin(), 3);
    EXPECT_EQ(&cv[1], first + 1);
    v.at(1) = 4;
    EXPECT_EQ(cv.at(1), 4);
    EXPECT_EQ(&v.front(), first);
    EXPECT_EQ(&cv.back(), first + 2);
    EXPECT_EQ(v.rend() - v.rbegin(), 3);
    *v.rbegin() = 7;
    EXPECT_EQ(*cv.crbegin(), 7);
    EXPECT_EQ(cv.crbegin(), cv.rbegin());
    EXPECT_EQ(cv.crend(), cv.rend());
    EXPECT_THROW(v.at(3), std::out_of_range);
    EXPECT_THROW(cv.at(8), std::out_of_range);
    v.reserve(8);
    v.shrink_to_fit();
    v.resize(5);
    EXPECT_EQ(v, (Ints{1, 4, 7, 0, 0}));
    v.resize(7, v[1]);
    EXPECT_EQ(v, (Ints{1, 4, 7, 0, 0, 4, 4}));
    v.resize(1);
    EXPECT_EQ(v.data(), first);
    EXPECT_EQ(v, (Ints{1}));
    EXPECT_THROW(v.resize(9), std::bad_alloc);
    EXPECT_THROW(v.reserve(9), std::bad_alloc);
    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_THROW(v.front(), std::out_of_range);
    EXPECT_THROW(cv.front(), std::out_of_range);
    EXPECT_THROW(v.back(), std::out_of_range);
    EXPECT_THROW(cv.back(), std::out_of_range);
    EXPECT_THROW(v.pop_back(), std::out_of_range);
}

TEST(InplaceVector, CheckedFallibleAndUncheckedAppends) {
    InplaceVector<std::string, 8> v;
    const std::string word = "one";
    EXPECT_EQ(&v.push_back(word), v.begin());
    EXPECT_EQ(&v.push_back(std::string("two")), v.begin() + 1);
    EXPECT_EQ(v.emplace_back(3, 'a'), "aaa");
    EXPECT_EQ(v.try_push_back(word), v.begin() + 3);
    EXPECT_EQ(v.try_push_back(std::string("five")), v.begin() + 4);
    EXPECT_EQ(v.try_emplace_back(3, 'b'), v.begin() + 5);
    EXPECT_EQ(v.unchecked_push_back(word), word);
    EXPECT_EQ(v.unchecked_push_back(std::string("eight")), "eight");
    EXPECT_EQ(v.try_emplace_back(3, 'z'), nullptr);
    EXPECT_EQ(v.try_push_back(word), nullptr);
    std::string pending = "pending";
    EXPECT_EQ(v.try_push_back(std::move(pending)), nullptr);
    EXPECT_THROW(v.push_back(std::move(pending)), std::bad_alloc);
    EXPECT_EQ(pending, "pending");
    EXPECT_THROW(v.emplace_back(3, 'z'), std::bad_alloc);
    v.pop_back();
    EXPECT_EQ(v.unchecked_emplace_back(3, 'c'), "ccc");
}

TEST(InplaceVector, PositionInsertionPreservesOrderAndHandlesAliasedElements) {
    using Strings = InplaceVector<std::string, 7>;
    Strings v{"ab", "cd"};
    EXPECT_EQ(v.emplace(v.cbegin() + 1, 3, 'x'), v.begin() + 1);
    EXPECT_EQ(v.insert(v.begin(), v.back()), v.begin());
    auto inserted = v.insert(v.end(), std::string("ef"));
    EXPECT_EQ(inserted, v.end() - 1);
    EXPECT_EQ(v.insert(v.begin() + 1, 2, v.back()), v.begin() + 1);
    EXPECT_EQ(v, (Strings{"cd", "ef", "ef", "ab", "xxx", "cd", "ef"}));
    EXPECT_EQ(v.insert(v.begin(), 0, v.front()), v.begin());
    auto original = v;
    EXPECT_THROW(v.insert(v.begin(), std::numeric_limits<std::size_t>::max(), v.front()),
                 std::bad_alloc);
    EXPECT_THROW(v.emplace(v.begin(), "full"), std::bad_alloc);
    EXPECT_EQ(v, original);
}

TEST(InplaceVector, RangeInsertionAndErasureReturnTheNextPosition) {
    Ints v{1, 6};
    const std::array middle{3, 4};
    EXPECT_EQ(v.insert(v.begin() + 1, middle.begin(), middle.end()), v.begin() + 1);
    EXPECT_EQ(v.insert(v.begin() + 1, {2}), v.begin() + 1);
    EXPECT_EQ(v.insert_range(v.end() - 1, std::views::iota(5, 6)), v.begin() + 4);
    EXPECT_EQ(v, (Ints{1, 2, 3, 4, 5, 6}));
    EXPECT_EQ(v.erase(v.begin() + 1, v.begin() + 1), v.begin() + 1);
    EXPECT_EQ(v.erase(v.begin()), v.begin());
    EXPECT_EQ(v.erase(v.begin() + 1, v.begin() + 3), v.begin() + 1);
    EXPECT_EQ(v, (Ints{2, 5, 6}));
    auto after_last = v.erase(v.end() - 1);
    EXPECT_EQ(after_last, v.end());
    v.append_range(std::array{2, 3, 2, 4});
    EXPECT_EQ(erase(v, 2), 3u);
    EXPECT_EQ(erase_if(v, [](int x) { return x % 2 != 0; }), 2u);
    EXPECT_EQ(v, (Ints{4}));
    EXPECT_EQ(erase(v, 42), 0u);
}

TEST(InplaceVector, SinglePassRangesAndCapacityOverflow) {
    std::istringstream input("1 2 3 4");
    InplaceVector<int, 2> first;
    auto values = std::ranges::istream_view<int>(input);
    auto remaining = first.try_append_range(values);
    EXPECT_EQ(first[0], 1);
    EXPECT_EQ(first.back(), 2);
    ASSERT_NE(remaining, values.end());
    EXPECT_EQ(*remaining, 3);
    Ints rest(std::move(remaining), values.end());
    EXPECT_EQ(rest, (Ints{3, 4}));
    const std::array more{5, 6};
    EXPECT_EQ(rest.try_append_range(more), more.end());
    EXPECT_EQ(rest, (Ints{3, 4, 5, 6}));
    std::istringstream oversized("7 8 9 10 11");
    EXPECT_THROW(rest.insert_range(rest.begin() + 1, std::ranges::istream_view<int>(oversized)),
                 std::bad_alloc);
    EXPECT_EQ(rest, (Ints{3, 4, 5, 6}));
    EXPECT_THROW(rest.append_range(std::array{1, 2, 3, 4, 5}), std::bad_alloc);
    EXPECT_EQ(rest, (Ints{3, 4, 5, 6}));
}

TEST(InplaceVector, ZeroCapacitySupportsEmptyRangesAndRejectsGrowth) {
    InplaceVector<int, 0> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.capacity(), 0u);
    EXPECT_EQ(v.begin(), v.end());
    EXPECT_EQ(v.rbegin(), v.rend());
    v.resize(0);
    v.reserve(0);
    v.clear();
    EXPECT_EQ(v.try_emplace_back(1), nullptr);
    EXPECT_THROW(v.push_back(1), std::bad_alloc);
    EXPECT_THROW(v.reserve(1), std::bad_alloc);
    EXPECT_EQ(v.erase(v.begin(), v.end()), v.end());
    EXPECT_EQ(v.insert(v.begin(), {}), v.end());
    const std::array one{1};
    EXPECT_EQ(v.try_append_range(one), one.begin());
}

TEST(InplaceVector, IteratorsCanConstructElementsWithExplicitConversions) {
    struct Item {
        explicit Item(int value) : value(value) {}
        int value;
    };
    const std::array values{4, 5};
    InplaceVector<Item, 4> items(values.begin(), values.end());
    items.insert(items.begin() + 1, values.begin(), values.end());
    EXPECT_EQ(items[0].value, 4);
    EXPECT_EQ(items[1].value, 4);
    EXPECT_EQ(items[2].value, 5);
    EXPECT_EQ(items[3].value, 5);
}

TEST(InplaceVector, MoveOnlyElementsAndUnequalSizeSwap) {
    Owners v;
    v.push_back(std::make_unique<int>(42));
    auto first = v.front().get();
    v.insert(v.begin(), std::make_unique<int>(7));
    Owners moved(std::move(v));
    EXPECT_EQ(moved.back().get(), first);
    Owners other;
    other.push_back(std::make_unique<int>(9));
    auto owned_by_other = other.front().get();
    swap(moved, other);
    ASSERT_EQ(moved.size(), 1u);
    ASSERT_EQ(other.size(), 2u);
    EXPECT_EQ(moved.front().get(), owned_by_other);
    EXPECT_EQ(*other.front(), 7);
    EXPECT_EQ(other.back().get(), first);
    other.swap(other);
    moved = std::move(other);
    EXPECT_EQ(moved.back().get(), first);
    EXPECT_EQ(moved.erase(moved.begin()), moved.begin());
    EXPECT_EQ(moved.front().get(), first);
}

TEST(InplaceVector, ElementLifetimesAndCopiesFollowLogicalSize) {
    int alive = 0;
    int copies_left = 100;
    {
        InplaceVector<Tracked, 5> v;
        EXPECT_EQ(alive, 0);
        auto &first = v.emplace_back(alive, copies_left, 1);
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&first) % alignof(Tracked), 0u);
        v.resize(3, first);
        EXPECT_EQ(alive, 3);
        auto copy = v;
        EXPECT_EQ(alive, 6);
        copy.pop_back();
        copy = v;
        EXPECT_EQ(alive, 6);
        copy.erase(copy.begin(), copy.end() - 1);
        EXPECT_EQ(alive, 4);
        v = copy;
        EXPECT_EQ(alive, 2);
        v.clear();
        EXPECT_EQ(alive, 1);
        v.emplace_back(alive, copies_left, 2);
        EXPECT_EQ(v.front().value, 2);
    }
    EXPECT_EQ(alive, 0);
}

TEST(InplaceVector, ThrowingCopiesRollBackGrowthAndDestroyPartialConstructions) {
    int alive = 0;
    int copies_left = 1;
    using Items = InplaceVector<Tracked, 4>;
    Items v;
    v.emplace_back(alive, copies_left, 11);
    EXPECT_THROW(v.resize(4, v.front()), std::runtime_error);
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(alive, 1);
    EXPECT_EQ(v.front().value, 11);
    EXPECT_THROW(v.try_push_back(v.front()), std::runtime_error);
    copies_left = 1;
    EXPECT_THROW((Items(3, v.front())), std::runtime_error);
    EXPECT_EQ(alive, 1);
    v.emplace_back(alive, copies_left, 12);
    copies_left = 1;
    EXPECT_THROW((Items(v)), std::runtime_error);
    EXPECT_EQ(alive, 2);
    copies_left = 1;
    EXPECT_THROW(v.insert(v.begin(), 2, v.back()), std::runtime_error);
    EXPECT_EQ(alive, 2);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v.back().value, 12);
}

TEST(InplaceVector, FallibleRangeAppendKeepsThePrefixOnElementFailure) {
    int alive = 0;
    int copies_left = 1;
    InplaceVector<Tracked, 3> source;
    source.emplace_back(alive, copies_left, 10);
    source.emplace_back(alive, copies_left, 20);
    InplaceVector<Tracked, 3> dest;
    dest.emplace_back(alive, copies_left, 1);
    EXPECT_THROW(dest.append_range(source), std::runtime_error);
    EXPECT_EQ(dest.size(), 1u);
    EXPECT_EQ(alive, 3);
    copies_left = 1;
    EXPECT_THROW(dest.try_append_range(source), std::runtime_error);
    ASSERT_EQ(dest.size(), 2u);
    EXPECT_EQ(dest.front().value, 1);
    EXPECT_EQ(dest.back().value, 10);
    EXPECT_EQ(alive, 4);
}

TEST(InplaceVector, ComparisonsAreLexicographic) {
    EXPECT_EQ((Ints{1, 2}), (Ints{1, 2}));
    EXPECT_NE((Ints{1}), (Ints{1, 2}));
    EXPECT_LT((Ints{1}), (Ints{1, 0}));
    EXPECT_GT((Ints{2}), (Ints{1, 9}));
    struct LessOnly {
        int value;
        bool operator<(const LessOnly &other) const { return value < other.value; }
    };
    using Legacy = InplaceVector<LessOnly, 2>;
    EXPECT_EQ((Legacy{{2}} <=> Legacy{{3}}), std::weak_ordering::less);
    using Floats = InplaceVector<double, 2>;
    EXPECT_EQ((Floats{std::numeric_limits<double>::quiet_NaN()} <=> Floats{1.0}),
              std::partial_ordering::unordered);
}

} // namespace
} // namespace prometheus::containers
