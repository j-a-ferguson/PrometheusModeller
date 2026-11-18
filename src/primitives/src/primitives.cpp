#include "primitives.hpp"
#include "schema.hpp"
#include "slot_pool.hpp"
#include <cstddef>

namespace prometheus::primitives {

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

    Builder(Body &body_in, size_t nv, size_t ne, size_t nfi, size_t nl, size_t nfaces, size_t ns,
            size_t nr, size_t n_poi, size_t n_cu, size_t n_su)
        : body{body_in}, vertices(nv), edges(ne), fins(nfi), loops(nl), faces(nfaces), shells(ns),
          regions(nr), points(n_poi), curves(n_cu), surfaces(n_su) {
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
    }
};

} // namespace

Handle<Body> make_cyliner(Modeller &modeller, CylinderForm form) {
    Handle<Body> handle = modeller.add_body();
    Body &body = handle.get();
    Builder b(body, 0, 2, 4, 4, 3, 2, 2, 0, 2, 3);

    // fin0  connections
    {
        Fin &fin = b.fins[0].get();
        fin.loop = b.loops[0];
        fin.edge = b.edges[0];
        fin.fin_prev = b.fins[0];
        fin.fin_next = b.fins[0];
        fin.vertex = Handle<Vertex>::null();
        fin.fin_next_at_vertex = b.fins[1];
        fin.edge = b.edges[0];
        fin.fin_next_at_vertex = Handle<Fin>::null();
        fin.sense = false;
    }

    // fin1  connections
    {
        Fin &fin = b.fins[1].get();
        fin.loop = b.loops[1];
        fin.edge = b.edges[0];
        fin.fin_prev = b.fins[1];
        fin.fin_next = b.fins[1];
        fin.vertex = Handle<Vertex>::null();
        fin.fin_next_at_vertex = b.fins[0];
        fin.edge = b.edges[0];
        fin.fin_next_at_vertex = Handle<Fin>::null();
        fin.sense = true;
    }

    return handle;
}

} // namespace prometheus::primitives