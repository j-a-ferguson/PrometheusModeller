

#include "utils.hpp"

#include <gtest/gtest.h>

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


}
