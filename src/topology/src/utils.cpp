

#include <prometheus/topology/utils.hpp>

#include <stdexcept>

namespace prometheus::topology {

    namespace  {
        
        void require_in_range(NodeId id, NodeId max_id) {
            if (id >= max_id) {
                throw std::logic_error("Index out of range");
            }
        }
    }


    IndexResource::IndexResource(): next_id_{0} {

    }



    NodeId IndexResource::peek_id() const {
        if (free_ids_.empty()) {
            return next_id_;
        }
        return *free_ids_.begin();
    }

    NodeId IndexResource::acquire_id() {

        if (free_ids_.empty()) {
            auto next_id = next_id_;
            ++next_id_;
            return next_id;
        }
        auto next_id = *free_ids_.begin();
        free_ids_.erase(free_ids_.begin());
        return next_id;
    }

    void IndexResource::release_id(NodeId id) {
        require_in_range(id, next_id_);
        free_ids_.insert(id);
    }

}
