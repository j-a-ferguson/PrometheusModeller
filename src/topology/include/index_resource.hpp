#pragma once

#include "node_id.hpp"

#include <set>

namespace prometheus::topology {

class IndexResource {

  public:
    IndexResource();
    NodeId peek_id() const;
    NodeId acquire_id();
    void release_id(NodeId);

  private:
    NodeId next_id_;
    std::set<NodeId> free_ids_;
};

} // namespace prometheus::topology
