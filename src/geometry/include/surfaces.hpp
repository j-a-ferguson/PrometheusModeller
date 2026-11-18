#ifndef SURFACES_HPP_
#define SURFACES_HPP_

#include "../../common/include/common.hpp"

#include <utility>

namespace prometheus::geometry {

class SurfaceBase {
public:
    virtual ~SurfaceBase() = default;

    virtual Vec3 eval(double u, double v) const = 0;
    // Orders (0, 0) evaluate the point. Negative orders throw std::invalid_argument.
    virtual Vec3 diff(double u, double v, int order_u, int order_v) const = 0;

    // Natural parameter patches; eval/diff do not clamp parameters to these ranges.
    virtual std::pair<double, double> param_range_u() const;
    bool in_range_u(double u) const;
    virtual std::pair<double, double> param_range_v() const;
    bool in_range_v(double v) const;

    virtual Vec3 tangent_u(double u, double v, bool normalize = true) const;
    virtual Vec3 tangent_v(double u, double v, bool normalize = true) const;
    // Normal orientation follows dS/du x dS/dv.
    virtual Vec3 normal(double u, double v, bool normalize = true) const;
    // Normalizing a zero or non-finite tangent/normal throws std::domain_error.
};

using Surface = std::unique_ptr<SurfaceBase>;

// Axes/normals must be unit vectors and radii/heights must be finite and positive.
// Constructors choose x_axis perpendicular to the axis, then y_axis = x_axis x axis,
// matching the reference. Angular parameters are in radians.
class Plane : public SurfaceBase {
public:
    Plane(const Vec3& origin, const Vec3& normal);

    Vec3 eval(double u, double v) const override;
    Vec3 diff(double u, double v, int order_u, int order_v) const override;

private:
    Vec3 origin_;
    Vec3 x_axis_;
    Vec3 y_axis_;
};

// u is the angle around the axis; v is axial distance in [0, height].
// The radius decreases linearly from radius_bottom to radius_top.
class Cone : public SurfaceBase {
public:
    // Requires radius_bottom > radius_top > 0, as in the reference frustum.
    Cone(const Vec3& center, const Vec3& axis, double height, double radius_bottom,
         double radius_top);

    Vec3 eval(double u, double v) const override;
    Vec3 diff(double u, double v, int order_u, int order_v) const override;
    std::pair<double, double> param_range_u() const override;
    std::pair<double, double> param_range_v() const override;

private:
    Vec3 center_;
    Vec3 axis_;
    Vec3 x_axis_;
    Vec3 y_axis_;
    double height_;
    double radius_bottom_;
    double tan_alpha_;
};

// u is the angle around the axis; v is unbounded axial distance.
class Cylinder : public SurfaceBase {
public:
    Cylinder(const Vec3& center, const Vec3& axis, double radius);

    Vec3 eval(double u, double v) const override;
    Vec3 diff(double u, double v, int order_u, int order_v) const override;
    std::pair<double, double> param_range_u() const override;

private:
    Vec3 center_;
    Vec3 axis_;
    Vec3 x_axis_;
    Vec3 y_axis_;
    double radius_;
};

// u runs around the axis; v runs around the tube, both in [0, 2*pi].
class Torus : public SurfaceBase {
public:
    // Default: center zero, radii 1 and 0.1, Cartesian axes, as in the reference.
    Torus() = default;
    Torus(const Vec3& center, const Vec3& axis, double major_radius, double minor_radius);

    Vec3 eval(double u, double v) const override;
    Vec3 diff(double u, double v, int order_u, int order_v) const override;
    std::pair<double, double> param_range_u() const override;
    std::pair<double, double> param_range_v() const override;

private:
    Vec3 center_{0.0, 0.0, 0.0};
    Vec3 x_axis_{1.0, 0.0, 0.0};
    Vec3 y_axis_{0.0, 1.0, 0.0};
    Vec3 z_axis_{0.0, 0.0, 1.0};
    double major_radius_ = 1.0;
    double minor_radius_ = 0.1;
};

// u is longitude in [0, 2*pi]; v is latitude in [-pi/2, pi/2].
class Sphere : public SurfaceBase {
public:
    // Default: unit sphere at the origin with Cartesian axes, as in the reference.
    Sphere() = default;
    Sphere(const Vec3& center, double radius, const Vec3& polar_axis);

    Vec3 eval(double u, double v) const override;
    Vec3 diff(double u, double v, int order_u, int order_v) const override;
    std::pair<double, double> param_range_u() const override;
    std::pair<double, double> param_range_v() const override;

    const Vec3& center() const { return center_; }
    double radius() const { return radius_; }

private:
    Vec3 center_{0.0, 0.0, 0.0};
    Vec3 x_axis_{1.0, 0.0, 0.0};
    Vec3 y_axis_{0.0, 1.0, 0.0};
    Vec3 z_axis_{0.0, 0.0, 1.0};
    double radius_ = 1.0;
};

} // namespace prometheus::geometry

#endif
