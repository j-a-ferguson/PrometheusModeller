#include <curves.hpp>

#include <gtest/gtest.h>

#include <numbers>
#include <stdexcept>

namespace {

using namespace prometheus;
using namespace prometheus::geometry;

constexpr double tolerance = 1e-12;
const Vec3 origin{1.0, -2.0, 3.0};
const Vec3 plane_normal{0.0, 0.0, 1.0};

void expect_vector_near(const Vec3& actual, const Vec3& expected) {
    for (arma::uword i = 0; i < 3; ++i) {
        EXPECT_NEAR(actual[i], expected[i], tolerance) << "component " << i;
    }
}

TEST(CurveLine, EvaluatesPoints) {
    const Line line(origin, Vec3{0.6, 0.8, 0.0});

    expect_vector_near(line.eval(0.0), origin);
    expect_vector_near(line.eval(0.5), {1.3, -1.6, 3.0});
    expect_vector_near(line.eval(-2.0), {-0.2, -3.6, 3.0});
}

TEST(CurveLine, DerivativesAreConstantOrZero) {
    const Line line(origin, Vec3{0.6, 0.8, 0.0});

    expect_vector_near(line.diff(0.5, 0), {1.3, -1.6, 3.0});
    expect_vector_near(line.diff(0.5, 1), {0.6, 0.8, 0.0});
    expect_vector_near(line.diff(0.5, 2), {0.0, 0.0, 0.0});
    expect_vector_near(line.diff(0.5, 4), {0.0, 0.0, 0.0});
    expect_vector_near(line.jacobian(0.5), {0.6, 0.8, 0.0});
}

TEST(CurveLine, TangentIsUnitAndFrenetFrameIsUndefined) {
    const Line line(origin, Vec3{0.6, 0.8, 0.0});

    expect_vector_near(line.tangent(0.5, false), {0.6, 0.8, 0.0});
    expect_vector_near(line.tangent(0.5), {0.6, 0.8, 0.0});
    expect_vector_near(line.normal(0.5, false), {0.0, 0.0, 0.0});
    expect_vector_near(line.binormal(0.5, false), {0.0, 0.0, 0.0});
    EXPECT_THROW(line.normal(0.5), std::domain_error);
    EXPECT_THROW(line.binormal(0.5), std::domain_error);
}

TEST(CurveCircle, EvaluatesCardinalPoints) {
    const Circle circle(origin, plane_normal, 2.0);
    constexpr double pi = std::numbers::pi;

    expect_vector_near(circle.eval(0.0), {1.0, 0.0, 3.0});
    expect_vector_near(circle.eval(pi / 2.0), {3.0, -2.0, 3.0});
    expect_vector_near(circle.eval(pi), {1.0, -4.0, 3.0});
    expect_vector_near(circle.eval(3.0 * pi / 2.0), {-1.0, -2.0, 3.0});
    expect_vector_near(circle.eval(2.0 * pi), {1.0, 0.0, 3.0});
}

TEST(CurveCircle, DerivativesRepeatEveryFourOrders) {
    const Circle circle(origin, plane_normal, 2.0);

    expect_vector_near(circle.diff(0.0, 0), {1.0, 0.0, 3.0});
    expect_vector_near(circle.diff(0.0, 1), {2.0, 0.0, 0.0});
    expect_vector_near(circle.diff(0.0, 2), {0.0, -2.0, 0.0});
    expect_vector_near(circle.diff(0.0, 3), {-2.0, 0.0, 0.0});
    expect_vector_near(circle.diff(0.0, 4), {0.0, 2.0, 0.0});
    expect_vector_near(circle.diff(0.0, 5), {2.0, 0.0, 0.0});
}

TEST(CurveCircle, FrameFollowsParameterization) {
    const Circle circle(origin, plane_normal, 2.0);

    expect_vector_near(circle.tangent(0.0, false), {2.0, 0.0, 0.0});
    expect_vector_near(circle.tangent(0.0), {1.0, 0.0, 0.0});
    expect_vector_near(circle.normal(0.0), {0.0, -1.0, 0.0});
    expect_vector_near(circle.binormal(0.0), {0.0, 0.0, -1.0});
}

TEST(CurveEllipse, EvaluatesCardinalPoints) {
    const Ellipse ellipse(origin, plane_normal, 2.0, 3.0);
    constexpr double pi = std::numbers::pi;

    expect_vector_near(ellipse.eval(0.0), {1.0, 0.0, 3.0});
    expect_vector_near(ellipse.eval(pi / 2.0), {4.0, -2.0, 3.0});
    expect_vector_near(ellipse.eval(pi), {1.0, -4.0, 3.0});
    expect_vector_near(ellipse.eval(3.0 * pi / 2.0), {-2.0, -2.0, 3.0});
}

TEST(CurveEllipse, DerivativesAndFrameFollowParameterization) {
    const Ellipse ellipse(origin, plane_normal, 2.0, 3.0);

    expect_vector_near(ellipse.diff(0.0, 0), {1.0, 0.0, 3.0});
    expect_vector_near(ellipse.diff(0.0, 1), {3.0, 0.0, 0.0});
    expect_vector_near(ellipse.diff(0.0, 2), {0.0, -2.0, 0.0});
    expect_vector_near(ellipse.diff(0.0, 3), {-3.0, 0.0, 0.0});
    expect_vector_near(ellipse.diff(0.0, 4), {0.0, 2.0, 0.0});
    expect_vector_near(ellipse.tangent(0.0), {1.0, 0.0, 0.0});
    expect_vector_near(ellipse.normal(0.0), {0.0, -1.0, 0.0});
    expect_vector_near(ellipse.binormal(0.0), {0.0, 0.0, -1.0});
}

TEST(CurveHyperbola, EvaluationAndDerivativesMatchReferenceValues) {
    const Hyperbola hyperbola(origin, plane_normal, 2.0, 3.0);

    expect_vector_near(hyperbola.eval(0.0), {1.0, 0.0, 3.0});
    expect_vector_near(hyperbola.eval(1.0), {4.525603580931405, 1.0861612696304874, 3.0});
    expect_vector_near(hyperbola.diff(1.0, 0), {4.525603580931405, 1.0861612696304874, 3.0});
    expect_vector_near(hyperbola.diff(0.0, 1), {3.0, 0.0, 0.0});
    expect_vector_near(hyperbola.diff(0.0, 2), {0.0, 2.0, 0.0});
    expect_vector_near(hyperbola.diff(1.0, 3), {4.629241904445731, 2.3504023872876028, 0.0});
    expect_vector_near(hyperbola.diff(1.0, 4), {3.525603580931404, 3.0861612696304874, 0.0});
}

TEST(CurveHyperbola, FrameFollowsParameterization) {
    const Hyperbola hyperbola(origin, plane_normal, 2.0, 3.0);

    expect_vector_near(hyperbola.tangent(0.0), {1.0, 0.0, 0.0});
    expect_vector_near(hyperbola.normal(0.0), {0.0, 1.0, 0.0});
    expect_vector_near(hyperbola.binormal(0.0), {0.0, 0.0, 1.0});
}

TEST(CurveParabola, EvaluationAndDerivativesMatchReferenceValues) {
    const Parabola parabola(origin, plane_normal, Vec3{1.0, 0.0, 0.0}, 0.5);

    expect_vector_near(parabola.eval(0.0), origin);
    expect_vector_near(parabola.eval(1.0), {3.0, -3.0, 3.0});
    expect_vector_near(parabola.eval(2.0), {5.0, -6.0, 3.0});
    expect_vector_near(parabola.eval(-1.0), {-1.0, -3.0, 3.0});
    expect_vector_near(parabola.diff(2.0, 0), {5.0, -6.0, 3.0});
    expect_vector_near(parabola.diff(2.0, 1), {2.0, -4.0, 0.0});
    expect_vector_near(parabola.diff(2.0, 2), {0.0, -2.0, 0.0});
    expect_vector_near(parabola.diff(2.0, 3), {0.0, 0.0, 0.0});
}

TEST(CurveParabola, FrameFollowsParameterization) {
    const Parabola parabola(origin, plane_normal, Vec3{1.0, 0.0, 0.0}, 0.5);

    expect_vector_near(parabola.tangent(0.0), {1.0, 0.0, 0.0});
    expect_vector_near(parabola.normal(0.0), {0.0, -1.0, 0.0});
    expect_vector_near(parabola.binormal(0.0), {0.0, 0.0, -1.0});
}

TEST(Curves, RejectInvalidInputs) {
    EXPECT_THROW(Line(origin, Vec3{3.0, 4.0, 0.0}), std::invalid_argument);
    EXPECT_THROW(Circle(origin, Vec3{0.0, 0.0, 0.0}, 1.0), std::invalid_argument);
    EXPECT_THROW(Circle(origin, plane_normal, 0.0), std::invalid_argument);
    EXPECT_THROW(Circle(origin, plane_normal, -1.0), std::invalid_argument);
    EXPECT_THROW(Ellipse(origin, plane_normal, 0.0, 1.0), std::invalid_argument);
    EXPECT_THROW(Hyperbola(origin, plane_normal, 1.0, 0.0), std::invalid_argument);
    EXPECT_THROW(Parabola(origin, plane_normal, plane_normal, 0.5), std::invalid_argument);

    const Line line(origin, Vec3{1.0, 0.0, 0.0});
    EXPECT_THROW(line.diff(0.0, -1), std::invalid_argument);
}

} // namespace
