
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

    body.get().for_each_fin([](Handle<Fin> fin){
        std::println("{}", fin.get());
    });
    body.get().for_each_edge([](Handle<Edge> edge){
        std::println("{}", edge.get());
    });

    
}

} // namespace prometheus::primitives
