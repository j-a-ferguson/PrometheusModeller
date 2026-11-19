#pragma once

#include <format>
#include <string_view>

#include <nlohmann/json.hpp>

namespace prometheus {

template <typename T> struct JsonFormatter : std::formatter<std::string_view, char> {
    template <typename FormatContext>
    auto format(const T &value, FormatContext &context) const {
        const auto text = nlohmann::json(value).dump();
        return std::formatter<std::string_view, char>::format(text, context);
    }
};

} // namespace prometheus
