#include "inplace_vector.hpp"
#include "schema.hpp"
#include "slot_pool.hpp"
#include "validate_internal.hpp"

#include <cstddef>

namespace prometheus::validate::internal {

using namespace containers;

#define VALIDATE(expr, body, res)                                                                  \
    expr(body, res);                                                                               \
    if (!res.is_ok()) {                                                                            \
        return res;                                                                                \
    }

void validate_fin_edge_links(Body &body, Result &res) {
    body.for_each_fin([&res](Handle<Fin> handle) {
        Fin &fin = handle.get();
        if (fin.edge.is_null()) {
            res.add_error(ErrorCode::EntityNull, fin, "edge");
        } else if (fin.edge.is_stale()) {
            res.add_error(ErrorCode::EntityStale, fin, "edge");
        }
    });
}

void validate_edge_fin_orient(Body &body, Result &res) {
    InplaceVector<Handle<Fin>, 8> fins;
    body.for_each_edge([&](Handle<Edge> edge) {

        edge.get().for_each_fin([&](auto fin) { 
            if(fin.get().edge != edge) {
                res.add_error(ErrorCode::EdgeFinLinkBroken, edge.get());
                return;
            }
            fins.push_back(fin); 
        });

        if (fins.size() != 2) {
            res.add_error(ErrorCode::EdgeTooManyFins, edge.get());
            return;
        }

        if (fins[0] == fins[1]) {
            res.add_error(ErrorCode::EdgeDuplicateFin, edge.get());
            return;
        }

        if (fins[0].get().vertex == fins[1].get().vertex) {
            res.add_error(ErrorCode::EdgeWrongFinOrientation, edge.get());
            return;
        }
    });
}

void validate_edge_fins(Body &body, Result &res) {
    validate_fin_edge_links(body, res);
    validate_edge_fin_orient(body, res);
}
//.................................................................................................


void validate_face_loops(Body& body, Result &res) {
    body.for_each_face([&](Handle<Face> face){
        face.get().for_each_loop([&](Handle<Loop> loop){
            if(loop.get().face != face) {
                res.add_error(ErrorCode::FaceLoopLinkBroken, face.get());
            }
        });
    });
}
//.................................................................................................

Result validate_manifold_solid(Body &body) {
    Result res;
    VALIDATE(validate_edge_fins, body, res)
    return res;
}

} // namespace prometheus::validate::internal
