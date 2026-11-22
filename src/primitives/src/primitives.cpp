#include "primitives.hpp"
#include "common.hpp"
#include "curves.hpp"
#include "operations.hpp"
#include "schema.hpp"
#include "slot_pool.hpp"
#include "surfaces.hpp"
#include <cstddef>

namespace prometheus::primitives {

void to_json(nlohmann::json &json, const CylinderForm &form) {
    json = {
        {"bottom", {form.bottom[0], form.bottom[1], form.bottom[2]}},
        {"axis", {form.axis[0], form.axis[1], form.axis[2]}},
        {"radius", form.radius},
        {"height", form.height},
    };
}

namespace {
struct Builder {

    Body &body;
    std::vector<Handle<Vertex>> vertices;
    std::vector<Handle<Edge>> edges;
    std::vector<Handle<Fin>> fins;
    std::vector<Handle<Loop>> loops;
    std::vector<Handle<Face>> faces;
    std::vector<Handle<Shell>> shells;
    std::vector<Handle<Region>> regions;
    std::vector<Handle<PointNode>> points;
    std::vector<Handle<CurveNode>> curves;
    std::vector<Handle<SurfaceNode>> surfaces;

    Builder(Body &body_in, size_t nvertices, size_t nedges, size_t nfins, size_t nloops, size_t nfaces, size_t nshells,
            size_t nregions, size_t npoints, size_t ncurves, size_t nsurfaces)
        : body{body_in}, vertices(nvertices), edges(nedges), fins(nfins), loops(nloops), faces(nfaces), shells(nshells),
          regions(nregions), points(npoints), curves(ncurves), surfaces(nsurfaces) {
        for (auto &vertex : vertices) {
            vertex = body.addVertex();
        }
        for (auto &edge : edges) {
            edge = body.addEdge();
        }
        for (auto &fin : fins) {
            fin = body.addFin();
        }
        for (auto &loop : loops) {
            loop = body.addLoop();
        }
        for (auto &face : faces) {
            face = body.addFace();
        }
        for (auto &shell : shells) {
            shell = body.addShell();
        }
        for (auto &region : regions) {
            region = body.addRegion();
        }
        for(auto& point_node: points) {
            point_node = body.addPoint();
        }
        for(auto& curve_node: curves) {
            curve_node = body.addCurve();
        }
        for(auto& surface_node: surfaces) {
            surface_node = body.addSurface();
        }
    }

    template <typename NodeType> std::pair<Handle<NodeType>, NodeType &> get(std::size_t idx) {
        auto pools = std::tie(vertices, edges, fins, loops, faces, shells, regions, points, curves,
                              surfaces);

        using Handles = std::vector<Handle<NodeType>>;
        auto &handle = std::get<Handles &>(pools).at(idx);
        return {handle, handle.get()};
    }
};

void make_cylinder_topology(Builder& b) {
    // two edges, each with two fins attached
    attach_fins_to_edge(b.edges[0], {{b.fins[0], false}, {b.fins[1], true}});
    attach_fins_to_edge(b.edges[1], {{b.fins[2], true}, {b.fins[3], false}});

    // each loop has a single fin and no vertices
    attach_fins_to_loop(b.loops[0], {b.fins[0]});
    attach_fins_to_loop(b.loops[1], {b.fins[1]});
    attach_fins_to_loop(b.loops[2], {b.fins[2]});
    attach_fins_to_loop(b.loops[3], {b.fins[3]});

    // bottom disc
    attach_loops_to_face(b.faces[0], {b.loops[0]}, 0);
    // side cylinder, no outer loop as periodic
    attach_loops_to_face(b.faces[1], {b.loops[1], b.loops[2]}, -1);
    // top disc
    attach_loops_to_face(b.faces[2], {b.loops[3]}, 0);

    // attach closed set of faces to shells
    attach_front_faces_to_shell(b.shells[0], b.faces);
    attach_back_faces_to_shell(b.shells[1], b.faces);

    // outer unbounded region
    attach_shells_to_region(b.regions[0], {b.shells[0]});
    b.regions[0].get().make_void();
    // inner bounded region
    attach_shells_to_region(b.regions[1], {b.shells[1]});
    b.regions[1].get().make_solid();
}

void make_cylinder_geometry(Builder& b, CylinderForm form) {
    Vec3 bottom_disc_center = form.bottom;
    Vec3 bottom_disc_normal = -form.axis;
    Vec3 top_disc_center = form.bottom + form.height * form.axis;
    Vec3 top_disc_normal = form.axis;
    // curves
    auto bottom_circle = make_curve<Circle>(bottom_disc_center, bottom_disc_normal, form.radius);
    auto top_circle = make_curve<Circle>(top_disc_center, top_disc_normal, form.radius);
    // surfaces
    auto bottom_disc = make_surface<Plane>(bottom_disc_center, bottom_disc_normal);
    auto cylinder_side = make_surface<Cylinder>(form.bottom, form.axis, form.height);
    auto top_disc = make_surface<Plane>(top_disc_center, top_disc_normal);

    attach_curve(b.curves[0], std::move(bottom_circle));
    attach_geometry(b.edges[0], b.curves[0]);
    attach_curve(b.curves[1], std::move(top_circle));
    attach_surface(b.surfaces[0], std::move(bottom_disc));
    attach_surface(b.surfaces[1], std::move(cylinder_side));
    attach_surface(b.surfaces[2], std::move(top_disc));
}

} // namespace

Handle<Body> make_cyliner(Modeller &modeller, CylinderForm form) {
    Handle<Body> handle = modeller.add_body();
    Builder b(handle.get(), 0, 2, 4, 4, 3, 2, 2, 0, 2, 3);
    make_cylinder_topology(b);
    make_cylinder_geometry(b, std::move(form));
    return handle;
}

} // namespace prometheus::primitives
