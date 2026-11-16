#include <geometry/curves.hpp>

#include <cmath>
#include <limits>
#include <stdexcept>

namespace prometheus::geometry {
namespace {

constexpr double unit_tolerance = 1e-12;

void require_finite(const Vec3& vector) {
    if (!vector.is_finite()) {
        throw std::invalid_argument("Curve vectors must contain finite values");
    }
}

void require_unit(const Vec3& vector) {
    require_finite(vector);
    if (std::abs(arma::norm(vector) - 1.0) >= unit_tolerance) {
        throw std::invalid_argument("Curve directions and normals must be unit vectors");
    }
}

void require_positive(double value) {
    if (!std::isfinite(value) || value <= 0.0) {
        throw std::invalid_argument("Curve radii and slopes must be finite and positive");
    }
}

void require_order(int order) {
    if (order < 0) {
        throw std::invalid_argument("Derivative order must be nonnegative");
    }
}

Vec3 unit_vector(const Vec3& vector) {
    const double length = arma::norm(vector);
    if (!std::isfinite(length) || length == 0.0) {
        throw std::domain_error("Cannot normalize a zero or non-finite curve frame vector");
    }
    return vector / length;
}

// The same deterministic perpendicular direction as the Rust reference.
Vec3 orthogonal_vector(const Vec3& normal) {
    require_unit(normal);
    const double sign = std::copysign(1.0, normal[0]);
    const double beta = 1.0 / (1.0 + std::abs(normal[0]));
    return Vec3{-sign * normal[1], 1.0 - beta * normal[1] * normal[1],
                -beta * normal[1] * normal[2]};
}

Vec3 periodic_derivative(double t, int order, const Vec3& x_axis, const Vec3& y_axis,
                         double x_radius, double y_radius) {
    const double c = std::cos(t);
    const double s = std::sin(t);
    switch (order % 4) {
    case 0:
        return x_radius * c * x_axis + y_radius * s * y_axis;
    case 1:
        return -x_radius * s * x_axis + y_radius * c * y_axis;
    case 2:
        return -x_radius * c * x_axis - y_radius * s * y_axis;
    default:
        return x_radius * s * x_axis - y_radius * c * y_axis;
    }
}

} // namespace

std::pair<double, double> Curve::param_range() const {
    const double infinity = std::numeric_limits<double>::infinity();
    return {-infinity, infinity};
}

bool Curve::in_range(double t) const {
    const auto [start, end] = param_range();
    return t >= start && t <= end;
}

Vec3 Curve::tangent(double t, bool normalize) const {
    const Vec3 velocity = diff(t, 1);
    return normalize ? unit_vector(velocity) : velocity;
}

Vec3 Curve::normal(double t, bool normalize) const {
    const Vec3 velocity = diff(t, 1);
    const Vec3 acceleration = diff(t, 2);
    const Vec3 normal = arma::cross(arma::cross(velocity, acceleration), velocity);
    return normalize ? unit_vector(normal) : normal;
}

Vec3 Curve::binormal(double t, bool normalize) const {
    const Vec3 binormal = arma::cross(diff(t, 1), diff(t, 2));
    return normalize ? unit_vector(binormal) : binormal;
}

Vec3 Curve::jacobian(double t) const {
    return diff(t, 1);
}

Line::Line(const Vec3& origin, const Vec3& direction)
    : origin_(origin), direction_(direction) {
    require_finite(origin);
    require_unit(direction);
}

Vec3 Line::eval(double t) const {
    return origin_ + t * direction_;
}

Vec3 Line::diff(double t, int order) const {
    require_order(order);
    if (order == 0) {
        return eval(t);
    }
    if (order == 1) {
        return direction_;
    }
    return Vec3(arma::fill::zeros);
}

Circle::Circle(const Vec3& center, const Vec3& normal, double radius)
    : center_(center), x_axis_(orthogonal_vector(normal)),
      y_axis_(arma::cross(x_axis_, normal)), radius_(radius) {
    require_finite(center);
    require_positive(radius);
}

Vec3 Circle::eval(double t) const {
    return center_ + periodic_derivative(t, 0, x_axis_, y_axis_, radius_, radius_);
}

Vec3 Circle::diff(double t, int order) const {
    require_order(order);
    if (order == 0) {
        return eval(t);
    }
    return periodic_derivative(t, order, x_axis_, y_axis_, radius_, radius_);
}

Ellipse::Ellipse(const Vec3& center, const Vec3& normal, double x_radius, double y_radius)
    : center_(center), x_axis_(orthogonal_vector(normal)),
      y_axis_(arma::cross(x_axis_, normal)), x_radius_(x_radius), y_radius_(y_radius) {
    require_finite(center);
    require_positive(x_radius);
    require_positive(y_radius);
}

Vec3 Ellipse::eval(double t) const {
    return center_ + periodic_derivative(t, 0, x_axis_, y_axis_, x_radius_, y_radius_);
}

Vec3 Ellipse::diff(double t, int order) const {
    require_order(order);
    if (order == 0) {
        return eval(t);
    }
    return periodic_derivative(t, order, x_axis_, y_axis_, x_radius_, y_radius_);
}

Hyperbola::Hyperbola(const Vec3& center, const Vec3& normal, double x_radius, double y_radius)
    : center_(center), x_axis_(orthogonal_vector(normal)),
      y_axis_(arma::cross(x_axis_, normal)), x_radius_(x_radius), y_radius_(y_radius) {
    require_finite(center);
    require_positive(x_radius);
    require_positive(y_radius);
}

Vec3 Hyperbola::eval(double t) const {
    return center_ + x_radius_ * std::cosh(t) * x_axis_ + y_radius_ * std::sinh(t) * y_axis_;
}

Vec3 Hyperbola::diff(double t, int order) const {
    require_order(order);
    if (order == 0) {
        return eval(t);
    }
    if (order % 2 == 0) {
        return x_radius_ * std::cosh(t) * x_axis_ + y_radius_ * std::sinh(t) * y_axis_;
    }
    return x_radius_ * std::sinh(t) * x_axis_ + y_radius_ * std::cosh(t) * y_axis_;
}

Parabola::Parabola(const Vec3& origin, const Vec3& normal, const Vec3& direction, double slope)
    : origin_(origin), x_axis_(direction), y_axis_(arma::cross(direction, normal)), slope_(slope) {
    require_finite(origin);
    require_unit(normal);
    require_unit(direction);
    require_positive(slope);
    if (std::abs(arma::dot(normal, direction)) >= unit_tolerance) {
        throw std::invalid_argument("Parabola direction must be perpendicular to its normal");
    }
}

Vec3 Parabola::eval(double t) const {
    return origin_ + 4.0 * slope_ * t * x_axis_ + t * t * y_axis_;
}

Vec3 Parabola::diff(double t, int order) const {
    require_order(order);
    switch (order) {
    case 0:
        return eval(t);
    case 1:
        return 4.0 * slope_ * x_axis_ + 2.0 * t * y_axis_;
    case 2:
        return 2.0 * y_axis_;
    default:
        return Vec3(arma::fill::zeros);
    }
}

} // namespace prometheus::geometry
