#include <geometry/utils.hpp>

#include <cmath>
#include <stdexcept>

namespace prometheus::geometry::utils {

void require_finite(const Vec3& vector) {
    if (!vector.is_finite()) {
        throw std::invalid_argument("Geometry vectors must contain finite values");
    }
}

void require_unit(const Vec3& vector) {
    require_finite(vector);
    if (std::abs(arma::norm(vector) - 1.0) >= unit_tolerance) {
        throw std::invalid_argument("Geometry directions, axes and normals must be unit vectors");
    }
}

void require_positive(double value) {
    if (!std::isfinite(value) || value <= 0.0) {
        throw std::invalid_argument("Geometry dimensions must be finite and positive");
    }
}

void require_order(int order) {
    if (order < 0) {
        throw std::invalid_argument("Derivative orders must be nonnegative");
    }
}

void require_orders(int order_u, int order_v) {
    require_order(order_u);
    require_order(order_v);
}

Vec3 unit_vector(const Vec3& vector) {
    const double length = arma::norm(vector);
    if (!std::isfinite(length) || length == 0.0) {
        throw std::domain_error("Cannot normalize a zero or non-finite geometry vector");
    }
    return vector / length;
}

// The same deterministic perpendicular direction as the Rust reference.
Vec3 orthogonal_vector(const Vec3& axis) {
    require_unit(axis);
    const double sign = std::copysign(1.0, axis[0]);
    const double beta = 1.0 / (1.0 + std::abs(axis[0]));
    return Vec3{-sign * axis[1], 1.0 - beta * axis[1] * axis[1],
                -beta * axis[1] * axis[2]};
}

double cos_derivative(double t, int order) {
    switch (order % 4) {
    case 0:
        return std::cos(t);
    case 1:
        return -std::sin(t);
    case 2:
        return -std::cos(t);
    default:
        return std::sin(t);
    }
}

double sin_derivative(double t, int order) {
    switch (order % 4) {
    case 0:
        return std::sin(t);
    case 1:
        return std::cos(t);
    case 2:
        return -std::sin(t);
    default:
        return -std::cos(t);
    }
}

Vec3 periodic_derivative(double t, int order, const Vec3& x_axis, const Vec3& y_axis,
                         double x_radius, double y_radius) {
    return x_radius * cos_derivative(t, order) * x_axis
           + y_radius * sin_derivative(t, order) * y_axis;
}

Vec3 latitude_derivative(double u, int order, const Vec3& x_axis, const Vec3& y_axis) {
    return periodic_derivative(u, order, x_axis, y_axis, 1.0, 1.0);
}

} // namespace prometheus::geometry::utils
