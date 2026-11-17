#include <curves.hpp>
#include "utils.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace prometheus::geometry {

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
    return normalize ? utils::unit_vector(velocity) : velocity;
}

Vec3 Curve::normal(double t, bool normalize) const {
    const Vec3 velocity = diff(t, 1);
    const Vec3 acceleration = diff(t, 2);
    const Vec3 normal = arma::cross(arma::cross(velocity, acceleration), velocity);
    return normalize ? utils::unit_vector(normal) : normal;
}

Vec3 Curve::binormal(double t, bool normalize) const {
    const Vec3 binormal = arma::cross(diff(t, 1), diff(t, 2));
    return normalize ? utils::unit_vector(binormal) : binormal;
}

Vec3 Curve::jacobian(double t) const {
    return diff(t, 1);
}
//..................................................................................................

Line::Line(const Vec3& origin, const Vec3& direction)
    : origin_(origin), direction_(direction) {
    utils::require_finite(origin);
    utils::require_unit(direction);
}

Vec3 Line::eval(double t) const {
    return origin_ + t * direction_;
}

Vec3 Line::diff(double t, int order) const {
    utils::require_order(order);
    if (order == 0) {
        return eval(t);
    }
    if (order == 1) {
        return direction_;
    }
    return Vec3(arma::fill::zeros);
}
//..................................................................................................

Circle::Circle(const Vec3& center, const Vec3& normal, double radius)
    : center_(center), x_axis_(utils::orthogonal_vector(normal)),
      y_axis_(arma::cross(x_axis_, normal)), radius_(radius) {
    utils::require_finite(center);
    utils::require_positive(radius);
}

Vec3 Circle::eval(double t) const {
    return center_ + utils::periodic_derivative(t, 0, x_axis_, y_axis_, radius_, radius_);
}

Vec3 Circle::diff(double t, int order) const {
    utils::require_order(order);
    if (order == 0) {
        return eval(t);
    }
    return utils::periodic_derivative(t, order, x_axis_, y_axis_, radius_, radius_);
}
//..................................................................................................

Ellipse::Ellipse(const Vec3& center, const Vec3& normal, double x_radius, double y_radius)
    : center_(center), x_axis_(utils::orthogonal_vector(normal)),
      y_axis_(arma::cross(x_axis_, normal)), x_radius_(x_radius), y_radius_(y_radius) {
    utils::require_finite(center);
    utils::require_positive(x_radius);
    utils::require_positive(y_radius);
}

Vec3 Ellipse::eval(double t) const {
    return center_ + utils::periodic_derivative(t, 0, x_axis_, y_axis_, x_radius_, y_radius_);
}

Vec3 Ellipse::diff(double t, int order) const {
    utils::require_order(order);
    if (order == 0) {
        return eval(t);
    }
    return utils::periodic_derivative(t, order, x_axis_, y_axis_, x_radius_, y_radius_);
}
//..................................................................................................

Hyperbola::Hyperbola(const Vec3& center, const Vec3& normal, double x_radius, double y_radius)
    : center_(center), x_axis_(utils::orthogonal_vector(normal)),
      y_axis_(arma::cross(x_axis_, normal)), x_radius_(x_radius), y_radius_(y_radius) {
    utils::require_finite(center);
    utils::require_positive(x_radius);
    utils::require_positive(y_radius);
}

Vec3 Hyperbola::eval(double t) const {
    return center_ + x_radius_ * std::cosh(t) * x_axis_ + y_radius_ * std::sinh(t) * y_axis_;
}

Vec3 Hyperbola::diff(double t, int order) const {
    utils::require_order(order);
    if (order == 0) {
        return eval(t);
    }
    if (order % 2 == 0) {
        return x_radius_ * std::cosh(t) * x_axis_ + y_radius_ * std::sinh(t) * y_axis_;
    }
    return x_radius_ * std::sinh(t) * x_axis_ + y_radius_ * std::cosh(t) * y_axis_;
}
//..................................................................................................

Parabola::Parabola(const Vec3& origin, const Vec3& normal, const Vec3& direction, double slope)
    : origin_(origin), x_axis_(direction), y_axis_(arma::cross(direction, normal)), slope_(slope) {
    utils::require_finite(origin);
    utils::require_unit(normal);
    utils::require_unit(direction);
    utils::require_positive(slope);
    if (std::abs(arma::dot(normal, direction)) >= utils::unit_tolerance) {
        throw std::invalid_argument("Parabola direction must be perpendicular to its normal");
    }
}

Vec3 Parabola::eval(double t) const {
    return origin_ + 4.0 * slope_ * t * x_axis_ + t * t * y_axis_;
}

Vec3 Parabola::diff(double t, int order) const {
    utils::require_order(order);
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
//..................................................................................................

} // namespace prometheus::geometry
