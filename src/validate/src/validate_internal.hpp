#ifndef VALIDATE_INTERNAL_HPP_
#define VALIDATE_INTERNAL_HPP_

#include "schema.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace prometheus::validate::internal {
using namespace topology;
using namespace geometry;

#define ERROR_LIST(X)                                                                              \
    X(EntityNull, "is null")                                                                       \
    X(EntityStale, "is stale")                                                                     \
    X(EdgeTooManyFins, "has more than 2 fins")                                                     \
    X(EdgeWrongFinOrientation, "has fins with the same direction")

enum class ErrorCode : std::uint16_t {
#define X(code, msg) code,
    ERROR_LIST(X)
#undef X
};

struct ErrorInfo {
    std::string_view name;
    std::string_view message;
};

inline constexpr ErrorInfo error_table[]{
#define X(code, msg) ErrorInfo(#code, msg),
    ERROR_LIST(X)
#undef X
};

constexpr const ErrorInfo &info(ErrorCode code) { return error_table[static_cast<size_t>(code)]; }

constexpr std::string_view node_name(const Edge &) { return "Edge"; }
constexpr std::string_view node_name(const Fin &) { return "Fin"; }

struct NodeDetails {
    std::string_view name;
    NodeId id;
};

struct Diagnostic {
    ErrorCode code;
    NodeDetails node;
    std::string member;

    [[nodiscard]] std::string message() const {
        const auto description = info(code).message;
        if (member.empty()) {
            return std::format("{} {} {}", node.name, node.id, description);
        }
        return std::format("{} {}.{} {}", node.name, node.id, member, description);
    }
};

struct Result {
    std::vector<Diagnostic> errors;

    [[nodiscard]] bool is_ok() const { return errors.empty(); }

    template <typename Node>
    void add_error(ErrorCode code, const Node &node, std::string_view member = {}) {
        errors.push_back({code, {node_name(node), node.id}, std::string(member)});
    }
};

Result validate_manifold_solid(Body &body);
Result validate_manifold_sheet(Body &body);
Result validate_manifold_wire(Body &body);
Result validate_general(Body &body);
} // namespace prometheus::validate::internal

#endif
