

#include <prometheus/topology/utils.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace prometheus::topology {

TEST(Utils, IndexResource) {

    IndexResource resource;

    for (int i = 0; i < 200; ++i) {
        auto idx = resource.acquire_id();
        ASSERT_EQ(idx, i);
    }

    for (NodeId i = 20; i < 50; ++i) {
        resource.release_id(i);
    }

    for (int i = 0; i < 30; ++i) {
        ASSERT_EQ(resource.acquire_id(), 20 + i);
    }

    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(resource.acquire_id(), 200 + i);
    }
}

TEST(Utils, BasicUse) {

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

TEST(Utils, SlotPoolGetReturnsMutableAndConstReferences) {
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

TEST(Utils, ErasedHandleStaysStaleAfterSlotReuse) {
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

TEST(Utils, RejectsHandleWithOutOfRangeIndex) {
    SlotPool<int> pool;
    const auto live = pool.emplace(42);
    const Handle invalid(pool, live.index() + 1, live.gen());
    const auto &const_pool = pool;

    EXPECT_TRUE(pool.is_stale(invalid));
    EXPECT_FALSE(pool.get(invalid).has_value());
    EXPECT_FALSE(const_pool.get(invalid).has_value());
    EXPECT_FALSE(pool.erase(invalid));
}

TEST(Utils, RejectsHandleMatchingFreeSlotGeneration) {
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

TEST(Utils, EmptyAndFullyErasedPoolsHaveNoElements) {
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

TEST(Utils, IteratorsSkipFreeSlotsAndReportCurrentHandles) {
    struct Entry {
        int value;
    };
    SlotPool<Entry> pool;
    const auto leading = pool.emplace(Entry{0});
    const auto first = pool.emplace(Entry{10});
    const auto middle = pool.emplace(Entry{20});
    const auto old = pool.emplace(Entry{30});
    const auto trailing = pool.emplace(Entry{40});

    ASSERT_TRUE(pool.erase(old));
    const auto replacement = pool.emplace(Entry{99});
    ASSERT_EQ(replacement.index(), old.index());
    ASSERT_NE(replacement.gen(), old.gen());

    ASSERT_TRUE(pool.erase(leading));
    ASSERT_TRUE(pool.erase(middle));
    ASSERT_TRUE(pool.erase(trailing));

    auto it = pool.begin();
    ASSERT_NE(it, pool.end());
    EXPECT_EQ(it.handle().index(), first.index());
    EXPECT_EQ(it.handle().gen(), first.gen());
    it->value = 11;

    auto previous = it++;
    EXPECT_EQ((*previous).value, 11);
    ASSERT_NE(it, pool.end());
    EXPECT_EQ(it.handle().index(), replacement.index());
    EXPECT_EQ(it.handle().gen(), replacement.gen());
    EXPECT_EQ(it->value, 99);
    EXPECT_EQ(++it, pool.end());

    const auto &const_pool = pool;
    auto cit = const_pool.cbegin();
    static_assert(std::is_same_v<decltype(*cit), const Entry &>);
    ASSERT_NE(cit, const_pool.cend());
    EXPECT_EQ(cit->value, 11);
    ++cit;
    ASSERT_NE(cit, const_pool.cend());
    EXPECT_EQ(cit->value, 99);
    EXPECT_EQ(++cit, const_pool.cend());
}

} // namespace prometheus::topology
