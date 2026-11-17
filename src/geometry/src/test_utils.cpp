#include "utils.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace prometheus::geometry::utils {

TEST(GeometryUtils, NormalizesVectorsAndRejectsZero) {
    const Vec3 unit = unit_vector(Vec3{3.0, 4.0, 0.0});

    EXPECT_NEAR(unit[0], 0.6, unit_tolerance);
    EXPECT_NEAR(unit[1], 0.8, unit_tolerance);
    EXPECT_DOUBLE_EQ(unit[2], 0.0);
    EXPECT_THROW(unit_vector(Vec3{0.0, 0.0, 0.0}), std::domain_error);
}

} // namespace prometheus::geometry::utils
