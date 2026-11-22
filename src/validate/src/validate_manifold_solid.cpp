#include "schema.hpp"
#include "slot_pool.hpp"
#include "inplace_vector.hpp"
#include "validate_internal.hpp"

#include <cstddef>



namespace prometheus::validate::internal {

using namespace containers;

void validate_fin_edge_links(Body &body, Result &res) {
    body.for_each_fin([&res](Handle<Fin> handle) {
        const Fin &fin = handle.get();
        if (fin.edge.is_null()) {
            res.add_error(ErrorCode::EntityNull, fin, "edge");
        } else if (fin.edge.is_stale()) {
            res.add_error(ErrorCode::EntityStale, fin, "edge");
        }
    });
}

void validate_edge_fins_counts(Body &body, Result &res) {
    body.for_each_edge([&res](Handle<Edge> handle) {
        Edge &edge = handle.get();
        std::size_t count = 0;
        edge.for_each_fin([&count](Handle<Fin>) { ++count; });
        if (count > 2) {
            res.add_error(ErrorCode::EdgeTooManyFins, edge);
        }
    });
}

void validate_edge_fin_orient(Body& body, Result& res) 
{
    InplaceVector<Handle<Fin>, 16> fins; 
    body.for_each_edge([&](Handle<Edge> edge){
        edge.get().for_each_fin([&](auto fin){
            fins.push_back(fin);
        });

        if (fins.size() != 2 || fins[0].get().vertex == fins[1].get().vertex) {
            res.add_error(ErrorCode::EdgeWrongFinOrientation, edge.get());
        }
    });
}

void validate_edge_fins(Body& body, Result& res) {
    validate_edge_fins_counts(body, res);

}

Result validate_manifold_solid(Body &body) {
    Result res;
    validate_fin_edge_links(body, res);
    if(!res.is_ok()) {
        return res;
    }

    validate_edge_fins(body, res);
    return res;
}


}
