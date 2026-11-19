
#include "common.hpp"
#include "primitives.hpp"
#include "schema.hpp"

#include <gtest/gtest.h>

namespace prometheus::primitives {

TEST(Primitives, make_cylinder) {

    topology::Modeller modeller;
    auto body = make_cyliner(modeller, CylinderForm{.axis = Vec3{0.0, 0.0, 1.0},
                                                    .bottom = Vec3{0.0, 0.0, 0.0},
                                                    .height = 5.0,
                                                    .radius = 1.0});
}

} // namespace prometheus::primitives