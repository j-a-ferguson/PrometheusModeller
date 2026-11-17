#pragma once

#include <cstddef>
#include <limits>

namespace prometheus::topology {

using NodeId = std::size_t;

inline constexpr NodeId NULL_ID = std::numeric_limits<NodeId>::max();

constexpr bool is_null(NodeId id) {
    return id == NULL_ID;
}

} // namespace prometheus::topology
