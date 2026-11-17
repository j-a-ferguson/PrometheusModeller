#ifndef CURVES_HPP_
#define CURVES_HPP_

#include <prometheus/common.hpp>

#include <utility>

namespace prometheus::geometry {

class Curve {
public:
    virtual ~Curve() = default;

    virtual Vec3 eval(double t) const = 0;
    // Order zero evaluates the point. Negative orders throw std::invalid_argument.
    virtual Vec3 diff(double t, int order = 1) const = 0;

    virtual std::pair<double, double> param_range() const;
    bool in_range(double t) const;

    // With normalize=false, returns C'(t).
    virtual Vec3 tangent(double t, bool normalize = true) const;
    // With normalize=false, returns (C'(t) x C''(t)) x C'(t).
    virtual Vec3 normal(double t, bool normalize = true) const;
    // With normalize=false, returns C'(t) x C''(t).
    virtual Vec3 binormal(double t, bool normalize = true) const;
    // Normalizing a zero frame vector throws std::domain_error. In particular,
    // a line has no Frenet normal or binormal; its unnormalized vectors are zero.

    // The 3-by-1 Jacobian of t -> C(t).
    virtual Vec3 jacobian(double t) const;
};

class Line : public Curve {
public:
    // Direction must be a unit vector.
    Line(const Vec3& origin, const Vec3& direction);

    Vec3 eval(double t) const override;
    Vec3 diff(double t, int order = 1) const override;

private:
    Vec3 origin_;
    Vec3 direction_;
};

// For planar curves, normal must be a unit vector and radii must be positive.
// As in the reference implementation, x_axis is chosen perpendicular to normal
// and y_axis = x_axis x normal. Circle/ellipse parameters are angles in radians.
class Circle : public Curve {
public:
    Circle(const Vec3& center, const Vec3& normal, double radius);

    Vec3 eval(double t) const override;
    Vec3 diff(double t, int order = 1) const override;

private:
    Vec3 center_;
    Vec3 x_axis_;
    Vec3 y_axis_;
    double radius_;
};

class Ellipse : public Curve {
public:
    Ellipse(const Vec3& center, const Vec3& normal, double x_radius, double y_radius);

    Vec3 eval(double t) const override;
    Vec3 diff(double t, int order = 1) const override;

private:
    Vec3 center_;
    Vec3 x_axis_;
    Vec3 y_axis_;
    double x_radius_;
    double y_radius_;
};

// The positive branch: center + rx*cosh(t)*x_axis + ry*sinh(t)*y_axis.
class Hyperbola : public Curve {
public:
    Hyperbola(const Vec3& center, const Vec3& normal, double x_radius, double y_radius);

    Vec3 eval(double t) const override;
    Vec3 diff(double t, int order = 1) const override;

private:
    Vec3 center_;
    Vec3 x_axis_;
    Vec3 y_axis_;
    double x_radius_;
    double y_radius_;
};

// origin + 4*slope*t*direction + t*t*(direction x normal).
class Parabola : public Curve {
public:
    // Normal and direction must be perpendicular unit vectors; slope is positive.
    Parabola(const Vec3& origin, const Vec3& normal, const Vec3& direction, double slope);

    Vec3 eval(double t) const override;
    Vec3 diff(double t, int order = 1) const override;

private:
    Vec3 origin_;
    Vec3 x_axis_;
    Vec3 y_axis_;
    double slope_;
};

} // namespace prometheus::geometry

#endif
