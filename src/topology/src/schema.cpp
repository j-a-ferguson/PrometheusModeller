#include "schema.hpp"
#include "curves.hpp"
#include "node_id.hpp"
#include "slot_pool.hpp"
#include "surfaces.hpp"
#include <memory>
#include <stdexcept>
#include <utility>

namespace prometheus::topology {

Point &PointNode::geo() { return point; }

const Point &PointNode::geo() const { return point; }
//.................................................................................................

CurveNode::CurveNode(NodeId node_id, std::unique_ptr<geometry::CurveBase> geometry_value)
    : id(node_id), curve(std::move(geometry_value)) {
    if (!curve) {
        throw std::invalid_argument("CurveNode requires a curve");
    }
}

Vec3 CurveNode::eval(double t) const { return curve->eval(t); }

Vec3 CurveNode::diff(double t, int order) const { return curve->diff(t, order); }

std::pair<double, double> CurveNode::param_range() const { return curve->param_range(); }

Vec3 CurveNode::tangent(double t, bool normalize) const { return curve->tangent(t, normalize); }

Vec3 CurveNode::normal(double t, bool normalize) const { return curve->normal(t, normalize); }

Vec3 CurveNode::binormal(double t, bool normalize) const { return curve->binormal(t, normalize); }

Vec3 CurveNode::jacobian(double t) const { return curve->jacobian(t); }

geometry::CurveBase &CurveNode::geo() { return *curve; }

const geometry::CurveBase &CurveNode::geo() const { return *curve; }
//.................................................................................................

SurfaceNode::SurfaceNode(NodeId node_id, std::unique_ptr<geometry::SurfaceBase> geometry_value)
    : id(node_id), surface(std::move(geometry_value)) {
    if (!surface) {
        throw std::invalid_argument("SurfaceNode requires a surface");
    }
}

Vec3 SurfaceNode::eval(double u, double v) const { return surface->eval(u, v); }

Vec3 SurfaceNode::diff(double u, double v, int order_u, int order_v) const {
    return surface->diff(u, v, order_u, order_v);
}

std::pair<double, double> SurfaceNode::param_range_u() const { return surface->param_range_u(); }

std::pair<double, double> SurfaceNode::param_range_v() const { return surface->param_range_v(); }

Vec3 SurfaceNode::tangent_u(double u, double v, bool normalize) const {
    return surface->tangent_u(u, v, normalize);
}

Vec3 SurfaceNode::tangent_v(double u, double v, bool normalize) const {
    return surface->tangent_v(u, v, normalize);
}

Vec3 SurfaceNode::normal(double u, double v, bool normalize) const {
    return surface->normal(u, v, normalize);
}

geometry::SurfaceBase &SurfaceNode::geo() { return *surface; }

const geometry::SurfaceBase &SurfaceNode::geo() const { return *surface; }
//.................................................................................................

const Point &Vertex::point() { return point_node.get().geo(); }
//.................................................................................................

template <typename T> Handle<T> Body::addTopology(SlotPool<T> &pool) {
    if (!modeller) {
        throw std::logic_error("Body has no owning modeller");
    }
    auto handle = pool.emplace();
    auto &node = handle.get();
    node.body = this;
    node.id = modeller->node_id_resource_.acquire_id();
    return handle;
}

Handle<Vertex> Body::addVertex() { return addTopology(vertices); }

Handle<Edge> Body::addEdge() { return addTopology(edges); }

Handle<Fin> Body::addFin() { return addTopology(fins); }

Handle<Loop> Body::addLoop() { return addTopology(loops); }

Handle<Face> Body::addFace() { return addTopology(faces); }

Handle<Shell> Body::addShell() { return addTopology(shells); }

Handle<Region> Body::addRegion() { return addTopology(regions); }

Handle<PointNode> Body::addPoint(Point point) {
    if (!modeller) {
        throw std::logic_error("Body has no owning modeller");
    }
    auto handle = points.emplace(point);
    auto &node = handle.get();
    node.body = this;
    node.id = modeller->node_id_resource_.acquire_id();
    return handle;
}
Handle<CurveNode> Body::addCurve(geometry::Curve curve) {
    if (!modeller) {
        throw std::logic_error("Body has no owning modeller");
    }
    auto node_id = modeller->node_id_resource_.acquire_id();
    auto handle = curves.emplace(node_id, std::move(curve));
    auto &node = handle.get();
    node.body = this;
    return handle;
}
Handle<SurfaceNode> Body::addSurface(geometry::Surface surface) {

    if (!modeller) {
        throw std::logic_error("Body has no owning modeller");
    }
    auto node_id = modeller->node_id_resource_.acquire_id();
    auto handle = surfaces.emplace(node_id, std::move(surface));
    auto &node = handle.get();
    node.body = this;
    return handle;
}
//.................................................................................................

Handle<Body> Modeller::add_body() {

    Handle<Body> handle = bodies.emplace();
    handle.get().modeller = this;
    handle.get().id = node_id_resource_.acquire_id();
    return handle;
}
//.................................................................................................

} // namespace prometheus::topology
