#include "schema.hpp"
#include "slot_pool.hpp"
// #include "inplace_vector.hpp"
#include "validate_internal.hpp"

#include <algorithm>
#include <cstddef>
#include <unordered_map>



namespace prometheus::validate::internal {

void validate_null_or_stale(Body &body, Result &res) {
    body.for_each_node([&res](auto handle) {
        if (handle.is_null()) {
            res.add_error(ErrorCode::EntityNull);
        } else if (handle.is_stale()) {
            res.add_error(ErrorCode::EntityStale);
        }
    });
}

void validate_edge_fins_counts(Body& body, Result& res)
{
    std::unordered_map<Handle<Fin>, size_t> fin_counts; 
    body.for_each_edge([&](Handle<Edge> edge){
        edge.get().for_each_fin([&](Handle<Fin> fin){
            fin_counts[fin] += 1;       
        });
    });
    for(const auto& [fin, count] : fin_counts) {
        if(count != 1) {
            res.add_error(ErrorCode::EdgeTooManyFins);
        }
    }
}

void validate_edge_fin_orient(Body& body, Result& res) 
{
    std::vector<Handle<Fin>> fins;
    fins.reserve(5);

    body.for_each_edge([&](Handle<Edge> edge){

        // edge.get().for_each_fin([&](Handle<Fin> fin){
        //     fin_counts[fin] += 1;       
        // });
    });
}

void validate_edge_fins(Body& body, Result& res) {
    validate_edge_fins_counts(body, res);

}

Result validate_manifold_solid(Body &body) {
    Result res;
    validate_null_or_stale(body, res);
    if(!res.is_ok()) {
        return res;
    }

    validate_edge_fins(body, res);
    return res;
}


}
