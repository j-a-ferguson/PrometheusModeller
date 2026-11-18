#ifndef SCHEMA_HPP_
#define SCHEMA_HPP_

#include <memory>

#include "chains.hpp"
#include "common.hpp"
#include "curves.hpp"
#include "index_resource.hpp"
#include "node_id.hpp"
#include "slot_pool.hpp"
#include "surfaces.hpp"

namespace prometheus::topology {

struct Vertex;
struct Edge;
struct Fin;
struct Loop;
struct Face;
struct Shell;
struct Region;
struct Body;
class Modeller;

using Point = Vec3;
//.................................................................................................

struct PointNode {
    PointNode(Point value) : point{std::move(value)} {}

    Point &geo();
    const Point &geo() const;

    Body *body;
    NodeId id;
    Point point;
};
//.................................................................................................

struct CurveNode : public geometry::CurveBase {
    CurveNode(NodeId node_id, std::unique_ptr<geometry::CurveBase> geometry_value);

    Vec3 eval(double t) const override;
    Vec3 diff(double t, int order = 1) const override;
    std::pair<double, double> param_range() const override;
    Vec3 tangent(double t, bool normalize = true) const override;
    Vec3 normal(double t, bool normalize = true) const override;
    Vec3 binormal(double t, bool normalize = true) const override;
    Vec3 jacobian(double t) const override;

    CurveBase &geo();
    const CurveBase &geo() const;

    Body *body;
    NodeId id;
    std::unique_ptr<geometry::CurveBase> curve;
};
//.................................................................................................

struct SurfaceNode : public geometry::SurfaceBase {
    SurfaceNode(NodeId node_id, std::unique_ptr<geometry::SurfaceBase> geometry_value);

    Vec3 eval(double u, double v) const override;
    Vec3 diff(double u, double v, int order_u, int order_v) const override;
    std::pair<double, double> param_range_u() const override;
    std::pair<double, double> param_range_v() const override;
    Vec3 tangent_u(double u, double v, bool normalize = true) const override;
    Vec3 tangent_v(double u, double v, bool normalize = true) const override;
    Vec3 normal(double u, double v, bool normalize = true) const override;

    SurfaceBase &geo();
    const SurfaceBase &geo() const;

    Body *body;
    NodeId id;
    std::unique_ptr<geometry::SurfaceBase> surface;
};
//.................................................................................................

struct Vertex {
    const Point &point();

    Body *body;
    // UID of node
    NodeId id;
    // Point of vertex
    Handle<PointNode> point_node;
    // Head of singly-linked chain of fins referencing this vertex
    Handle<Fin> fin;
};
//.................................................................................................

struct Edge {
    Body *body;
    // UID of node
    NodeId id;
    // Head of singly-linked chain of fins around edge
    Handle<Fin> fin;
    // Underlying curve
    Handle<CurveNode> curve;
    // Orientation relative to curve
    bool sense;
};

//.................................................................................................

struct Fin {
    Body *body;
    // UID of node
    NodeId id;
    // Loop of fin
    Handle<Loop> loop;
    // Prev fin around loop
    Handle<Fin> fin_prev;
    // Next fin around loop
    Handle<Fin> fin_next;
    // Forward vertex of fin
    Handle<Vertex> vertex;
    // Next fin around edge (clockwise looking along edge)
    Handle<Fin> fin_next_on_edge;
    // Edge of the fin
    Handle<Edge> edge;
    // Next fin in list referencing `vertex`
    Handle<Fin> fin_next_at_vertex;
    // Orientation relative to to the edge
    bool sense;
};

// head of chain is Vertex::fin;
using VertexFins = SinglyLinked<Fin, &Fin::fin_next_at_vertex>;
// head of chain is Edge::fin;
using EdgeFins = SinglyLinked<Fin, &Fin::fin_next_on_edge>;
// start of circular chain is Loop::fin
using LoopFins = DoublyLinked<Fin, &Fin::fin_next, &Fin::fin_prev, ChainShape::circular>;
//.................................................................................................

struct Loop {
    Body *body;
    // UID of node
    NodeId id;
    // Arbitrary fin in loop
    Handle<Fin> fin;
    // Next loop in face
    Handle<Loop> loop_next_in_face;
    // Owning face
    Handle<Face> face;
};

// start of chain is Face::loop
using FaceLoops = SinglyLinked<Loop, &Loop::loop_next_in_face>;
//.................................................................................................

struct Face {
    Body *body;
    // UID of node
    NodeId id;
    // head of chain of loops
    Handle<Loop> loop;
    // Previous back-face of shell
    Handle<Face> face_prev_in_back_shell;
    // Next back-face of shell
    Handle<Face> face_next_in_back_shell;
    // Previous front-face of shell
    Handle<Face> face_prev_in_front_shell;
    // Next front-face of shell
    Handle<Face> face_next_in_front_shell;
    // Shell for which this is a back-face
    Handle<Shell> shell_back;
    // Shell for which this is a front-face
    Handle<Shell> shell_front;
    // Previous face on surface
    Handle<Face> face_prev_on_surface;
    // Next face on surface
    Handle<Face> face_next_on_surface;
    // Surface
    Handle<SurfaceNode> surface;
    // Orientation w.r.t surface
    bool sense;
};

// head of chain is Shell::faces_back
using ShellBackFaces = DoublyLinked<Face, &Face::face_next_in_back_shell,
                                    &Face::face_prev_in_back_shell, ChainShape::linear>;
// head of chain is Shell::faces_front
using ShellFrontFaces = DoublyLinked<Face, &Face::face_next_in_front_shell,
                                     &Face::face_prev_in_front_shell, ChainShape::linear>;
//.................................................................................................

struct Shell {
    Body *body;
    // UID of node
    NodeId id;
    // Prev Shell in Region
    Handle<Shell> shell_prev_in_region;
    // Next Shell in Region
    Handle<Shell> shell_next_in_region;
    // Head of linked-list of back faces
    Handle<Face> faces_back;
    // Head of linked-list of front-faces
    Handle<Face> faces_front;
    // Region of shell
    Handle<Region> region;
};

// head of chain is Region::shells
using RegionShells = DoublyLinked<Shell, &Shell::shell_next_in_region, &Shell::shell_prev_in_region,
                                  ChainShape::linear>;
//.................................................................................................

enum class RegionType { Solid, Void };

struct Region {
    Body *body;
    // UID of node
    NodeId id;
    // Head of linked-list of shells
    Handle<Shell> shell;
    // Solid or void
    RegionType type;
};
//.................................................................................................

enum class BodyType { ManifoldSolid, ManifoldSheet, ManifoldWire, NonManifold };
//.................................................................................................

struct Body {
    Handle<Vertex> addVertex();
    Handle<Edge> addEdge();
    Handle<Fin> addFin();
    Handle<Loop> addLoop();
    Handle<Face> addFace();
    Handle<Shell> addShell();
    Handle<Region> addRegion();
    Handle<PointNode> addPoint(Point point);
    Handle<CurveNode> addCurve(geometry::Curve curve);
    Handle<SurfaceNode> addSurface(geometry::Surface surface);

    template <typename T> Handle<T> addTopology(SlotPool<T> &pool);

    Modeller *modeller{};

    NodeId id;

    // topology
    SlotPool<Vertex> vertices;
    SlotPool<Edge> edges;
    SlotPool<Fin> fins;
    SlotPool<Loop> loops;
    SlotPool<Face> faces;
    SlotPool<Shell> shells;
    SlotPool<Region> regions;

    // geometry
    SlotPool<PointNode> points;
    SlotPool<CurveNode> curves;
    SlotPool<SurfaceNode> surfaces;
};
//.................................................................................................

class Modeller {
    friend struct Body;

  public:
    Handle<Body> add_body();

  private:
    IndexResource node_id_resource_;
    SlotPool<Body> bodies;
};
//.................................................................................................

} // namespace prometheus::topology

#endif
