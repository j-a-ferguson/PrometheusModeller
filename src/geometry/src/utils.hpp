#ifndef GEOMETRY_UTILS_HPP_
#define GEOMETRY_UTILS_HPP_

#include <prometheus/common.hpp>

#include <numbers>

namespace prometheus::geometry::utils {

inline constexpr double unit_tolerance = 1e-12;
inline constexpr double two_pi = 2.0 * std::numbers::pi;

void require_finite(const Vec3& vector);
void require_unit(const Vec3& vector);
void require_positive(double value);
void require_order(int order);
void require_orders(int order_u, int order_v);

Vec3 unit_vector(const Vec3& vector);
Vec3 orthogonal_vector(const Vec3& axis);

// Derivative helpers expect nonnegative orders.
double cos_derivative(double t, int order);
double sin_derivative(double t, int order);
Vec3 periodic_derivative(double t, int order, const Vec3& x_axis, const Vec3& y_axis,
                         double x_radius, double y_radius);
Vec3 latitude_derivative(double u, int order, const Vec3& x_axis, const Vec3& y_axis);

} // namespace prometheus::geometry::utils

#endif
