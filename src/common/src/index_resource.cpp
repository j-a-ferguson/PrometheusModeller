#include "index_resource.hpp"

#include <stdexcept>

namespace prometheus::topology {

IndexResource::IndexResource() : next_id_{0} {}

NodeId IndexResource::peek_id() const {
    return free_ids_.empty() ? next_id_ : *free_ids_.begin();
}

NodeId IndexResource::acquire_id() {
    if (free_ids_.empty())
        return next_id_++;
    auto first = free_ids_.begin();
    auto id = *first;
    free_ids_.erase(first);
    return id;
}

void IndexResource::release_id(NodeId id) {
    if (id >= next_id_)
        throw std::logic_error("Index out of range");
    free_ids_.insert(id);
}

} // namespace prometheus::topology
