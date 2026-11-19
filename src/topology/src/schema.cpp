#include "schema.hpp"
#include "curves.hpp"
#include "node_id.hpp"
#include "slot_pool.hpp"
#include "surfaces.hpp"
#include <memory>
#include <stdexcept>
#include <utility>

namespace prometheus::topology {

void to_json(nlohmann::json &json, const PointNode &node) {
    json = {
        {"id", node.id},
        {"point", node.point},
    };
}

void to_json(nlohmann::json &json, const CurveNode &node) {
    json = {
        {"id", node.id},
        {"has_curve", static_cast<bool>(node.curve)},
    };
}

void to_json(nlohmann::json &json, const SurfaceNode &node) {
    json = {
        {"id", node.id},
        {"has_surface", static_cast<bool>(node.surface)},
    };
}

void to_json(nlohmann::json &json, const Vertex &node) {
    json = {
        {"id", node.id},
        {"point_node", node.point_node},
        {"fin", node.fin},
    };
}

void to_json(nlohmann::json &json, const Edge &node) {
    json = {
        {"id", node.id},
        {"fin", node.fin},
        {"curve", node.curve},
        {"sense", node.sense},
    };
}

void to_json(nlohmann::json &json, const Fin &node) {
    json = {
        {"id", node.id},
        {"loop", node.loop},
        {"fin_prev", node.fin_prev},
        {"fin_next", node.fin_next},
        {"vertex", node.vertex},
        {"fin_next_on_edge", node.fin_next_on_edge},
        {"edge", node.edge},
        {"fin_next_at_vertex", node.fin_next_at_vertex},
        {"sense", node.sense},
    };
}

void to_json(nlohmann::json &json, const Loop &node) {
    json = {
        {"id", node.id},
        {"fin", node.fin},
        {"loop_next_in_face", node.loop_next_in_face},
        {"face", node.face},
        {"is_outer", node.is_outer},
    };
}

void to_json(nlohmann::json &json, const Face &node) {
    json = {
        {"id", node.id},
        {"loop", node.loop},
        {"face_prev_in_back_shell", node.face_prev_in_back_shell},
        {"face_next_in_back_shell", node.face_next_in_back_shell},
        {"face_prev_in_front_shell", node.face_prev_in_front_shell},
        {"face_next_in_front_shell", node.face_next_in_front_shell},
        {"shell_back", node.shell_back},
        {"shell_front", node.shell_front},
        {"face_prev_on_surface", node.face_prev_on_surface},
        {"face_next_on_surface", node.face_next_on_surface},
        {"surface", node.surface},
        {"sense", node.sense},
    };
}

void to_json(nlohmann::json &json, const Shell &node) {
    json = {
        {"id", node.id},
        {"shell_prev_in_region", node.shell_prev_in_region},
        {"shell_next_in_region", node.shell_next_in_region},
        {"faces_back", node.faces_back},
        {"faces_front", node.faces_front},
        {"region", node.region},
    };
}

void to_json(nlohmann::json &json, const Region &node) {
    json = {
        {"id", node.id},
        {"shell", node.shell},
        {"type", node.type},
    };
}

void to_json(nlohmann::json &json, RegionType type) {
    switch (type) {
    case RegionType::Solid: json = "Solid"; return;
    case RegionType::Void: json = "Void"; return;
    }
    throw std::invalid_argument("Unknown RegionType");
}

void to_json(nlohmann::json &json, BodyType type) {
    switch (type) {
    case BodyType::ManifoldSolid: json = "ManifoldSolid"; return;
    case BodyType::ManifoldSheet: json = "ManifoldSheet"; return;
    case BodyType::ManifoldWire: json = "ManifoldWire"; return;
    case BodyType::General: json = "General"; return;
    }
    throw std::invalid_argument("Unknown BodyType");
}

Point &PointNode::geo() { return point; }

const Point &PointNode::geo() const { return point; }
//.................................................................................................

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
    if (!modeller_) {
        throw std::logic_error("Body has no owning modeller");
    }
    auto handle = pool.emplace();
    auto &node = handle.get();
    node.body = this;
    node.id = modeller_->node_id_resource_.acquire_id();
    return handle;
}

Handle<Vertex> Body::addVertex() { return addTopology(vertices_); }

Handle<Edge> Body::addEdge() { return addTopology(edges_); }

Handle<Fin> Body::addFin() { return addTopology(fins_); }

Handle<Loop> Body::addLoop() { return addTopology(loops_); }

Handle<Face> Body::addFace() { return addTopology(faces_); }

Handle<Shell> Body::addShell() { return addTopology(shells_); }

Handle<Region> Body::addRegion() { return addTopology(regions_); }

Handle<PointNode> Body::addPoint() {
    if (!modeller_) {
        throw std::logic_error("Body has no owning modeller");
    }
    auto handle = points_.emplace();
    auto &node = handle.get();
    node.body = this;
    node.id = modeller_->node_id_resource_.acquire_id();
    return handle;
}
Handle<CurveNode> Body::addCurve() {
    if (!modeller_) {
        throw std::logic_error("Body has no owning modeller");
    }
    auto handle = curves_.emplace();
    auto &node = handle.get();
    node.body = this;
    node.id = modeller_->node_id_resource_.acquire_id();
    return handle;
}
Handle<SurfaceNode> Body::addSurface() {

    if (!modeller_) {
        throw std::logic_error("Body has no owning modeller");
    }
    auto handle = surfaces_.emplace();
    auto &node = handle.get();
    node.body = this;
    node.id = modeller_->node_id_resource_.acquire_id();
    return handle;
}
//.................................................................................................

Handle<Body> Modeller::add_body() {

    Handle<Body> handle = bodies_.emplace();
    handle.get().modeller_ = this;
    handle.get().id_ = node_id_resource_.acquire_id();
    return handle;
}
//.................................................................................................

} // namespace prometheus::topology
