#ifndef VALIDATE_INTERNAL_HPP_
#define VALIDATE_INTERNAL_HPP_

#include "schema.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

namespace prometheus::validate::internal {
using namespace topology;
using namespace geometry;

#define ERROR_LIST(X)                                                                              \
    X(EntityNull, "Entity {} with id {} is null")                                                  \
    X(EntityStale, "Entity {} with id {} is stale")                                                 \
    X(EdgeTooManyFins, "Manifold edge {} has more than 2 fins")                                    \
    X(EdgeWrongFinOrientation, "Manifold edge {} fins with same direction")

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

struct Result {
    std::vector<ErrorInfo> errors;

    bool is_ok() { return errors.empty(); }
    void add_error(ErrorCode code) {
        errors.push_back(info(code));
    }
};

Result validate_manifold_solid(Body &body);
Result validate_manifold_sheet(Body &body);
Result validate_manifold_wire(Body &body);
Result validate_general(Body &body);
} // namespace prometheus::validate::internal

#endif