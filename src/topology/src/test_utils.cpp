

#include "utils.hpp"

#include <gtest/gtest.h>

#include <type_traits>

namespace prometheus::topology {

    TEST(Utils, IndexResource) {


        IndexResource resource;

        for(int i = 0; i < 200; ++i) {
            auto idx = resource.acquire_id();
            ASSERT_EQ(idx, i);
        }

        for(NodeId i = 20; i < 50; ++i) {
            resource.release_id(i);
        }

        for(int i = 0; i < 30; ++i) {
            ASSERT_EQ(resource.acquire_id(), 20 + i);
        }

        for(int i = 0; i < 10; ++i) {
            ASSERT_EQ(resource.acquire_id(), 200 + i);
        }
    }


    TEST(Utils, SlotPool) {

        auto pool = SlotPool<int>::with_capacity(100);

        for(int i = 0; i < 100; ++i) {
            auto handle = pool.emplace(i);
            ASSERT_TRUE(!pool.is_stale(handle));
            ASSERT_TRUE(pool.get(handle).has_value());
            ASSERT_TRUE(pool.get(handle).value() == i);
        }

        for(Index i = 20; i < 50; ++i) {
            Handle h{.gen=1, .index=i};
            ASSERT_TRUE(pool.erase(h));
        }

        int ii{0};
        for(Index i = 20; i < 50; ++i) {
            auto handle = pool.emplace(100 + i);
            ASSERT_TRUE(!pool.is_stale(handle));
            ASSERT_EQ(handle.gen, 3);
            ASSERT_EQ(handle.index,  49-ii);
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

        const auto& const_pool = pool;
        auto const_value = const_pool.get(handle);
        static_assert(std::is_same_v<decltype(const_value->get()), const int&>);
        ASSERT_TRUE(const_value.has_value());
        EXPECT_EQ(const_value->get(), 43);
        EXPECT_EQ(&const_value->get(), &value->get());
    }


}
