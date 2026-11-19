
#include "primitives.hpp"
#include "schema.hpp"
#include "slot_pool.hpp"

#include <gtest/gtest.h>

#include <print>
#include <sstream>

namespace prometheus::primitives {

TEST(Primitives, CylinderFormFormatsAsCompactJson) {
    static_assert(std::formattable<CylinderForm, char>);
    const CylinderForm form{.bottom = {2.0, -1.0, 4.0}, .axis = {0.36, 0.48, 0.8},
                            .radius = 1.25, .height = 5.0};
    const nlohmann::json expected = {
        {"bottom", {2.0, -1.0, 4.0}}, {"axis", {0.36, 0.48, 0.8}},
        {"radius", 1.25}, {"height", 5.0},
    };
    const nlohmann::json json = form;
    EXPECT_EQ(json, expected);
    EXPECT_EQ(std::format("{}", form), json.dump());
    EXPECT_EQ(std::format("{:>100}", form), std::format("{:>100}", json.dump()));

    std::ostringstream output;
    std::println(output, "{}", form);
    EXPECT_EQ(output.str(), json.dump() + "\n");
}

TEST(Primitives, make_cylinder) {

    topology::Modeller modeller;
    auto body = make_cyliner(modeller, CylinderForm{.axis = Vec3{0.0, 0.0, 1.0},
                                                    .bottom = Vec3{0.0, 0.0, 0.0},
                                                    .height = 5.0,
                                                    .radius = 1.0});

    auto &cylinder = body.get();
    const auto print_node = [](const auto &handle) { std::println("{}", handle.get()); };
    cylinder.for_each_vertex(print_node);
    cylinder.for_each_fin(print_node);
    cylinder.for_each_edge(print_node);
    cylinder.for_each_loop(print_node);
    cylinder.for_each_face(print_node);
    cylinder.for_each_shell(print_node);
    cylinder.for_each_region(print_node);
}

} // namespace prometheus::primitives
