#include "slot_pool.hpp"

#include <gtest/gtest.h>

#include <type_traits>

namespace prometheus::topology {

TEST(SlotPool, NullHandleFormatsAsJsonNull) {
    static_assert(std::formattable<Handle<int>, char>);
    const Handle<int> handle;
    const nlohmann::json json = handle;
    EXPECT_TRUE(json.is_null());
    EXPECT_EQ(std::format("{}", handle), json.dump());
}

TEST(SlotPool, HandleFormattingPreservesIdentityAfterSlotReuse) {
    SlotPool<int> pool;
    const auto original = pool.emplace(42);
    const nlohmann::json expected = {{"index", original.index()}, {"generation", original.gen()}};
    const nlohmann::json live = original;
    EXPECT_EQ(live, expected);
    EXPECT_EQ(std::format("{}", original), live.dump());

    ASSERT_TRUE(pool.erase(original));
    const auto replacement = pool.emplace(99);
    ASSERT_EQ(replacement.index(), original.index());
    ASSERT_NE(replacement.gen(), original.gen());
    EXPECT_EQ(nlohmann::json(original), live);
    EXPECT_EQ(std::format("{}", original), live.dump());

    const nlohmann::json new_live = replacement;
    EXPECT_EQ(new_live.at("index"), live.at("index"));
    EXPECT_EQ(new_live.at("generation"), replacement.gen());
    EXPECT_EQ(std::format("{}", replacement), new_live.dump());
}

TEST(SlotPool, BasicUse) {

    auto pool = SlotPool<int>::with_capacity(100);

    for (int i = 0; i < 100; ++i) {
        auto handle = pool.emplace(i);
        ASSERT_TRUE(!pool.is_stale(handle));
        ASSERT_TRUE(pool.get(handle).has_value());
        ASSERT_TRUE(pool.get(handle).value() == i);
    }

    for (Index i = 20; i < 50; ++i) {
        const Handle h(pool, i, 1);
        ASSERT_TRUE(pool.erase(h));
    }

    int ii{0};
    for (Index i = 20; i < 50; ++i) {
        auto handle = pool.emplace(100 + i);
        ASSERT_TRUE(!pool.is_stale(handle));
        ASSERT_EQ(handle.gen(), 3);
        ASSERT_EQ(handle.index(), 49 - ii);
        ASSERT_TRUE(pool.get(handle).has_value());
        ASSERT_TRUE(pool.get(handle).value() == 100 + i);
        ++ii;
    }
}

TEST(SlotPool, GetReturnsMutableAndConstReferences) {
    SlotPool<int> pool;
    const auto handle = pool.emplace(42);

    auto value = pool.get(handle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value->get(), 42);
    value->get() = 43;

    const auto &const_pool = pool;
    auto const_value = const_pool.get(handle);
    static_assert(std::is_same_v<decltype(const_value->get()), const int &>);
    ASSERT_TRUE(const_value.has_value());
    EXPECT_EQ(const_value->get(), 43);
    EXPECT_EQ(&const_value->get(), &value->get());
}

TEST(SlotPool, ErasedHandleStaysStaleAfterSlotReuse) {
    SlotPool<int> pool;
    const auto original = pool.emplace(42);

    ASSERT_TRUE(pool.erase(original));
    EXPECT_TRUE(pool.is_stale(original));
    EXPECT_FALSE(pool.get(original).has_value());
    ASSERT_FALSE(pool.erase(original));

    const auto replacement = pool.emplace(99);
    ASSERT_EQ(replacement.index(), original.index());
    EXPECT_NE(replacement.gen(), original.gen());

    EXPECT_TRUE(pool.is_stale(original));
    EXPECT_FALSE(pool.get(original).has_value());
    ASSERT_FALSE(pool.erase(original));

    const auto value = pool.get(replacement);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value->get(), 99);
}

TEST(SlotPool, RejectsHandleWithOutOfRangeIndex) {
    SlotPool<int> pool;
    const auto live = pool.emplace(42);
    const Handle invalid(pool, live.index() + 1, live.gen());
    const auto &const_pool = pool;

    EXPECT_TRUE(pool.is_stale(invalid));
    EXPECT_FALSE(pool.get(invalid).has_value());
    EXPECT_FALSE(const_pool.get(invalid).has_value());
    EXPECT_FALSE(pool.erase(invalid));
}

TEST(SlotPool, RejectsHandleMatchingFreeSlotGeneration) {
    SlotPool<int> pool;
    const auto live = pool.emplace(42);
    ASSERT_TRUE(pool.erase(live));

    // Match the generation after erase so only liveness distinguishes this slot.
    const Handle free_slot(pool, live.index(), live.gen() + 1);
    const auto &const_pool = pool;

    EXPECT_TRUE(pool.is_stale(free_slot));
    EXPECT_FALSE(pool.get(free_slot).has_value());
    EXPECT_FALSE(const_pool.get(free_slot).has_value());
    ASSERT_FALSE(pool.erase(free_slot));

    const auto replacement = pool.emplace(99);
    EXPECT_EQ(replacement.index(), free_slot.index());
    EXPECT_FALSE(pool.is_stale(replacement));
}

TEST(SlotPool, EmptyAndFullyErasedPoolsHaveNoElements) {
    SlotPool<int> pool;
    const auto &const_pool = pool;

    EXPECT_EQ(pool.begin(), pool.end());
    EXPECT_EQ(const_pool.cbegin(), const_pool.cend());

    const auto first = pool.emplace(10);
    const auto second = pool.emplace(20);
    ASSERT_TRUE(pool.erase(first));
    ASSERT_TRUE(pool.erase(second));

    EXPECT_EQ(pool.begin(), pool.end());
    EXPECT_EQ(const_pool.cbegin(), const_pool.cend());
}

TEST(SlotPool, IteratorsSkipFreeSlotsAndReportCurrentHandles) {
    SlotPool<Vec3> pool;
    const auto leading = pool.emplace(Vec3{0, 0, 0});
    const auto first = pool.emplace(Vec3{10, 0, 0});
    const auto middle = pool.emplace(Vec3{20, 0, 0});
    const auto old = pool.emplace(Vec3{30, 0, 0});
    const auto trailing = pool.emplace(Vec3{40, 0, 0});

    ASSERT_TRUE(pool.erase(old));
    const auto replacement = pool.emplace(Vec3{99, 0, 0});
    ASSERT_EQ(replacement.index(), old.index());
    ASSERT_NE(replacement.gen(), old.gen());

    ASSERT_TRUE(pool.erase(leading));
    ASSERT_TRUE(pool.erase(middle));
    ASSERT_TRUE(pool.erase(trailing));

    auto it = pool.begin();
    ASSERT_NE(it, pool.end());
    EXPECT_EQ(it.handle().index(), first.index());
    EXPECT_EQ(it.handle().gen(), first.gen());
    it->at(0) = 11;

    auto previous = it++;
    EXPECT_EQ((*previous)[0], 11);
    ASSERT_NE(it, pool.end());
    EXPECT_EQ(it.handle().index(), replacement.index());
    EXPECT_EQ(it.handle().gen(), replacement.gen());
    EXPECT_EQ(it->at(0), 99);
    EXPECT_EQ(++it, pool.end());

    const auto &const_pool = pool;
    auto cit = const_pool.cbegin();
    static_assert(std::is_same_v<decltype(*cit), const Vec3 &>);
    ASSERT_NE(cit, const_pool.cend());
    EXPECT_EQ(cit->at(0), 11);
    ++cit;
    ASSERT_NE(cit, const_pool.cend());
    EXPECT_EQ(cit->at(0), 99);
    EXPECT_EQ(++cit, const_pool.cend());
}

} // namespace prometheus::topology
