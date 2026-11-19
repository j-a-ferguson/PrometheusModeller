#ifndef OPERATIONS_HPP_
#define OPERATIONS_HPP_

#include "schema.hpp"
#include "slot_pool.hpp"
#include "curves.hpp"
#include "surfaces.hpp"

#include <stdexcept>
#include <vector>
#include <utility>

namespace prometheus::topology {

using namespace geometry;

using FinSense = std::pair<Handle<Fin>, bool>;


template <typename T1, typename T2>
void require_same_body(Handle<T1> h1, Handle<T2> h2) {
    if(h1.get().body == h2.get().body) {
        throw std::logic_error("Handles not from same pool");
    }
}

void attach_vertex_to_fin(Handle<Vertex> vertex, Handle<Fin> fin);

/*! Attaches the given fins to the given edges with the given senses relative to the edges 
    Assumes that whatever fin-to-vertex connection are present have already been made
*/
void attach_fins_to_edge(Handle<Edge> edge, std::vector<FinSense> fins_senses); 

/*! Attaches the given fins to the given loop, fins are assumed to be ordered counter-clockwisde
    around the loop.
*/
void attach_fins_to_loop(Handle<Loop> loop, std::vector<Handle<Fin>> fins);

/*! Attaches the given loops to the given face, assumes loop-vertex connections have
    been made
*/
void attach_loops_to_face(Handle<Face> face, std::vector<Handle<Loop>> loops, int outer);

void attach_front_faces_to_shell(Handle<Shell> shell, std::vector<Handle<Face>> faces);

void attach_back_faces_to_shell(Handle<Shell> shell, std::vector<Handle<Face>> faces);

void attach_shells_to_region(Handle<Region> region, std::vector<Handle<Shell>> shells);

void attach_geometry(Handle<PointNode> vertex, Point point);

void attach_geometry(Handle<CurveNode> vertex, Curve curve);

void attach_geometry(Handle<SurfaceNode> vertex, Surface surface);

} // namespace prometheus::topology

#endif