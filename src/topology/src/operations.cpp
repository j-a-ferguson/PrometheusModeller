#include "operations.hpp"
#include "schema.hpp"
#include "slot_pool.hpp"

#include <cassert>

namespace prometheus::topology {

int wrap(int i, size_t n) {
    assert(n > 0);
    const int r = i % n;
    return r < 0 ? r + n : r;
}

void attach_vertex_to_fin(Handle<Vertex> vertex, Handle<Fin> fin) {
    VertexFins::push_front(vertex.get().fin, fin);
    fin.get().vertex = vertex;
}

void attach_fins_to_edge(Handle<Edge> edge, std::vector<FinSense> fins_senses) {

    for (auto &[fin, sense] : fins_senses) {
        fin.get().sense = sense;
        EdgeFins::push_front(edge.get().fin, fin);
    }
}

void attach_fins_to_loop(Handle<Loop> loop, std::vector<Handle<Fin>> fins) {

    int n = fins.size();
    loop.get().fin = fins.front();
    int i{0};
    for (auto &fin : fins) {
        fin.get().fin_prev = fins[wrap(i - 1, n)];
        fin.get().fin_next = fins[wrap(i + 1, n)];
        fin.get().loop = loop;
        ++i;
    }
}

void attach_loops_to_face(Handle<Face> face, std::vector<Handle<Loop>> loops, int outer) {

    int i{0};
    for (auto &loop : loops) {
        bool is_outer = (outer == i);
        FaceLoops::push_front(face.get().loop, loop);
        loop.get().face = face;
        ++i;
    }
}

void attach_front_faces_to_shell(Handle<Shell> shell, std::vector<Handle<Face>> faces) {
    for (auto &face : faces) {
        face.get().shell_front = shell;
        ShellFrontFaces::push_front(shell.get().faces_front, face);
    }
}

void attach_back_faces_to_shell(Handle<Shell> shell, std::vector<Handle<Face>> faces) {
    for (auto &face : faces) {
        face.get().shell_back = shell;
        ShellBackFaces::push_front(shell.get().faces_back, face);
    }
}

void attach_shells_to_region(Handle<Region> region, std::vector<Handle<Shell>> shells) {
    for (auto &shell : shells) {
        shell.get().region = region;
        RegionShells::push_front(region.get().shell, shell);
    }
}

void attach_geometry(Handle<PointNode> point_node, Point point) { point_node.get().point = point; }

void attach_geometry(Handle<CurveNode> curve_node, Curve curve) {
    curve_node.get().curve = std::move(curve);
}

void attach_geometry(Handle<SurfaceNode> surface_node, Surface surface) {
    surface_node.get().surface = std::move(surface);
}

} // namespace prometheus::topology