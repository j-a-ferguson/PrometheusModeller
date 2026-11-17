#include <geometry/surfaces.hpp>
#include <geometry/utils.hpp>

#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace prometheus::geometry {

std::pair<double, double> Surface::param_range_u() const {
    const double infinity = std::numeric_limits<double>::infinity();
    return {-infinity, infinity};
}

bool Surface::in_range_u(double u) const {
    const auto [start, end] = param_range_u();
    return u >= start && u <= end;
}

std::pair<double, double> Surface::param_range_v() const {
    const double infinity = std::numeric_limits<double>::infinity();
    return {-infinity, infinity};
}

bool Surface::in_range_v(double v) const {
    const auto [start, end] = param_range_v();
    return v >= start && v <= end;
}

Vec3 Surface::tangent_u(double u, double v, bool normalize) const {
    const Vec3 tangent = diff(u, v, 1, 0);
    return normalize ? utils::unit_vector(tangent) : tangent;
}

Vec3 Surface::tangent_v(double u, double v, bool normalize) const {
    const Vec3 tangent = diff(u, v, 0, 1);
    return normalize ? utils::unit_vector(tangent) : tangent;
}

Vec3 Surface::normal(double u, double v, bool normalize) const {
    const Vec3 normal = arma::cross(diff(u, v, 1, 0), diff(u, v, 0, 1));
    return normalize ? utils::unit_vector(normal) : normal;
}

Plane::Plane(const Vec3& origin, const Vec3& normal)
    : origin_(origin), x_axis_(utils::orthogonal_vector(normal)),
      y_axis_(arma::cross(x_axis_, normal)) {
    utils::require_finite(origin);
}

Vec3 Plane::eval(double u, double v) const {
    return origin_ + u * x_axis_ + v * y_axis_;
}

Vec3 Plane::diff(double u, double v, int order_u, int order_v) const {
    utils::require_orders(order_u, order_v);
    if (order_u == 0 && order_v == 0) {
        return eval(u, v);
    }
    if (order_u == 1 && order_v == 0) {
        return x_axis_;
    }
    if (order_u == 0 && order_v == 1) {
        return y_axis_;
    }
    return Vec3(arma::fill::zeros);
}

Cone::Cone(const Vec3& center, const Vec3& axis, double height, double radius_bottom,
           double radius_top)
    : center_(center), axis_(axis), x_axis_(utils::orthogonal_vector(axis)),
      y_axis_(arma::cross(x_axis_, axis)), height_(height), radius_bottom_(radius_bottom) {
    utils::require_finite(center);
    utils::require_positive(height);
    utils::require_positive(radius_bottom);
    utils::require_positive(radius_top);
    if (radius_bottom <= radius_top) {
        throw std::invalid_argument("Cone bottom radius must be greater than its top radius");
    }
    tan_alpha_ = (radius_bottom - radius_top) / height;
}

Vec3 Cone::eval(double u, double v) const {
    const double radius = radius_bottom_ - v * tan_alpha_;
    return center_ + radius * utils::latitude_derivative(u, 0, x_axis_, y_axis_) + v * axis_;
}

Vec3 Cone::diff(double u, double v, int order_u, int order_v) const {
    utils::require_orders(order_u, order_v);
    if (order_u == 0 && order_v == 0) {
        return eval(u, v);
    }
    if (order_v == 0) {
        const double radius = radius_bottom_ - v * tan_alpha_;
        return radius * utils::latitude_derivative(u, order_u, x_axis_, y_axis_);
    }
    if (order_v == 1) {
        Vec3 derivative = -tan_alpha_ * utils::latitude_derivative(u, order_u, x_axis_, y_axis_);
        if (order_u == 0) {
            derivative += axis_;
        }
        return derivative;
    }
    return Vec3(arma::fill::zeros);
}

std::pair<double, double> Cone::param_range_u() const {
    return {0.0, utils::two_pi};
}

std::pair<double, double> Cone::param_range_v() const {
    return {0.0, height_};
}

Cylinder::Cylinder(const Vec3& center, const Vec3& axis, double radius)
    : center_(center), axis_(axis), x_axis_(utils::orthogonal_vector(axis)),
      y_axis_(arma::cross(x_axis_, axis)), radius_(radius) {
    utils::require_finite(center);
    utils::require_positive(radius);
}

Vec3 Cylinder::eval(double u, double v) const {
    return center_ + radius_ * utils::latitude_derivative(u, 0, x_axis_, y_axis_) + v * axis_;
}

Vec3 Cylinder::diff(double u, double v, int order_u, int order_v) const {
    utils::require_orders(order_u, order_v);
    if (order_u == 0 && order_v == 0) {
        return eval(u, v);
    }
    if (order_u == 0 && order_v == 1) {
        return axis_;
    }
    if (order_v == 0) {
        return radius_ * utils::latitude_derivative(u, order_u, x_axis_, y_axis_);
    }
    return Vec3(arma::fill::zeros);
}

std::pair<double, double> Cylinder::param_range_u() const {
    return {0.0, utils::two_pi};
}

Torus::Torus(const Vec3& center, const Vec3& axis, double major_radius, double minor_radius)
    : center_(center), x_axis_(utils::orthogonal_vector(axis)), y_axis_(arma::cross(x_axis_, axis)),
      z_axis_(axis), major_radius_(major_radius), minor_radius_(minor_radius) {
    utils::require_finite(center);
    utils::require_positive(major_radius);
    utils::require_positive(minor_radius);
    if (major_radius <= minor_radius) {
        throw std::invalid_argument("Torus major radius must be greater than its minor radius");
    }
}

Vec3 Torus::eval(double u, double v) const {
    return center_ + (major_radius_ + minor_radius_ * std::cos(v))
                         * utils::latitude_derivative(u, 0, x_axis_, y_axis_)
           + minor_radius_ * std::sin(v) * z_axis_;
}

Vec3 Torus::diff(double u, double v, int order_u, int order_v) const {
    utils::require_orders(order_u, order_v);
    if (order_u == 0 && order_v == 0) {
        return eval(u, v);
    }
    const double radial_factor = order_v == 0 ? major_radius_ + minor_radius_ * std::cos(v)
                                              : minor_radius_ * utils::cos_derivative(v, order_v);
    Vec3 derivative = radial_factor * utils::latitude_derivative(u, order_u, x_axis_, y_axis_);
    if (order_u == 0) {
        derivative += minor_radius_ * utils::sin_derivative(v, order_v) * z_axis_;
    }
    return derivative;
}

std::pair<double, double> Torus::param_range_u() const {
    return {0.0, utils::two_pi};
}

std::pair<double, double> Torus::param_range_v() const {
    return {0.0, utils::two_pi};
}

Sphere::Sphere(const Vec3& center, double radius, const Vec3& polar_axis)
    : center_(center), x_axis_(utils::orthogonal_vector(polar_axis)),
      y_axis_(arma::cross(x_axis_, polar_axis)), z_axis_(polar_axis), radius_(radius) {
    utils::require_finite(center);
    utils::require_positive(radius);
}

Vec3 Sphere::eval(double u, double v) const {
    return center_ + radius_ * std::cos(v) * utils::latitude_derivative(u, 0, x_axis_, y_axis_)
           + radius_ * std::sin(v) * z_axis_;
}

Vec3 Sphere::diff(double u, double v, int order_u, int order_v) const {
    utils::require_orders(order_u, order_v);
    if (order_u == 0 && order_v == 0) {
        return eval(u, v);
    }
    Vec3 derivative = radius_ * utils::cos_derivative(v, order_v)
                      * utils::latitude_derivative(u, order_u, x_axis_, y_axis_);
    // The polar component is independent of u, so all of its u derivatives vanish.
    if (order_u == 0) {
        derivative += radius_ * utils::sin_derivative(v, order_v) * z_axis_;
    }
    return derivative;
}

std::pair<double, double> Sphere::param_range_u() const {
    return {0.0, utils::two_pi};
}

std::pair<double, double> Sphere::param_range_v() const {
    return {-std::numbers::pi / 2.0, std::numbers::pi / 2.0};
}

} // namespace prometheus::geometry
