#include "schema.hpp"

#include <gtest/gtest.h>
#include <print>
#include <sstream>
#include <stdexcept>

namespace prometheus::topology {
namespace {

template <typename T> void expect_json(const T &value, const nlohmann::json &expected) {
    const nlohmann::json json = value;
    EXPECT_EQ(json, expected);
    EXPECT_EQ(std::format("{}", value), expected.dump());
}

TEST(SchemaJson, GeometryNodesReportCoordinatesAndAttachments) {
    Modeller modeller;
    auto &body = modeller.add_body().get();
    auto &point = body.addPoint().get();
    point.point = {3.5, -2.0, 1.0};
    expect_json(point, {{"id", point.id}, {"point", {3.5, -2.0, 1.0}}});

    auto &curve = body.addCurve().get();
    expect_json(curve, {{"id", curve.id}, {"has_curve", false}});
    curve.curve = geometry::make_curve<geometry::Line>(point.point, Vec3{0, 0, 1});
    expect_json(curve, {{"id", curve.id}, {"has_curve", true}});

    auto &surface = body.addSurface().get();
    expect_json(surface, {{"id", surface.id}, {"has_surface", false}});
    surface.surface = geometry::make_surface<geometry::Sphere>();
    expect_json(surface, {{"id", surface.id}, {"has_surface", true}});
}

TEST(SchemaJson, TopologyLinksUseNodeIdsEvenInCycles) {
    Modeller modeller;
    auto &body = modeller.add_body().get();
    const auto point = body.addPoint();
    const auto curve = body.addCurve();
    auto vertex = body.addVertex();
    auto edge = body.addEdge();
    auto fin = body.addFin();
    auto loop = body.addLoop();

    vertex.get().point_node = point;
    vertex.get().fin = fin;
    edge.get().fin = fin;
    edge.get().curve = curve;
    edge.get().sense = true;
    fin.get().loop = loop;
    fin.get().vertex = vertex;
    fin.get().edge = edge;
    LoopFins::push_front(loop.get().fin, fin);
    loop.get().is_outer = true;

    expect_json(vertex.get(), {{"id", vertex.get().id}, {"point_node", point.get().id},
                               {"fin", fin.get().id}});
    expect_json(edge.get(), {{"id", edge.get().id}, {"fin", fin.get().id},
                             {"curve", curve.get().id}, {"sense", true}});
    expect_json(fin.get(), {{"id", fin.get().id}, {"loop", loop.get().id},
                            {"fin_prev", fin.get().id}, {"fin_next", fin.get().id},
                            {"vertex", vertex.get().id}, {"fin_next_on_edge", nullptr},
                            {"edge", edge.get().id}, {"fin_next_at_vertex", nullptr}, {"sense", false}});
    expect_json(loop.get(), {{"id", loop.get().id}, {"fin", fin.get().id}, {"loop_next_in_face", nullptr},
                             {"face", nullptr}, {"is_outer", true}});
}

TEST(SchemaJson, StaleLinkCannotReportReplacementNodeId) {
    SlotPool<Fin> fins;
    Edge edge{};
    edge.fin = fins.emplace();
    edge.fin.get().id = 42;
    EXPECT_EQ(nlohmann::json(edge).at("fin"), 42);

    ASSERT_TRUE(fins.erase(edge.fin));
    auto replacement = fins.emplace();
    ASSERT_EQ(replacement.index(), edge.fin.index());
    replacement.get().id = 99;
    EXPECT_THROW(static_cast<void>(std::format("{}", edge)), std::logic_error);
}

TEST(SchemaJson, FaceShellAndRegionReportMembershipAndOrientation) {
    Modeller modeller;
    auto &body = modeller.add_body().get();
    auto face = body.addFace();
    auto shell = body.addShell();
    auto region = body.addRegion();
    auto surface = body.addSurface();
    face.get().shell_front = shell;
    face.get().surface = surface;
    face.get().sense = true;
    ShellFrontFaces::push_front(shell.get().faces_front, face);
    shell.get().region = region;
    RegionShells::push_front(region.get().shell, shell);
    region.get().make_void();

    expect_json(face.get(), {{"id", face.get().id}, {"loop", nullptr},
                             {"face_prev_in_back_shell", nullptr}, {"face_next_in_back_shell", nullptr},
                             {"face_prev_in_front_shell", nullptr}, {"face_next_in_front_shell", nullptr},
                             {"shell_back", nullptr}, {"shell_front", shell.get().id},
                             {"face_prev_on_surface", nullptr}, {"face_next_on_surface", nullptr},
                             {"surface", surface.get().id}, {"sense", true}});
    expect_json(shell.get(), {{"id", shell.get().id}, {"shell_prev_in_region", nullptr},
                              {"shell_next_in_region", nullptr}, {"faces_back", nullptr},
                              {"faces_front", face.get().id}, {"region", region.get().id}});
    expect_json(region.get(), {{"id", region.get().id}, {"shell", shell.get().id}, {"type", "Void"}});

    std::ostringstream output;
    std::println(output, "{}", region.get());
    EXPECT_EQ(output.str(), nlohmann::json(region.get()).dump() + "\n");
}

TEST(SchemaJson, EnumValuesUseTheirDeclaredNames) {
    expect_json(RegionType::Solid, "Solid");
    expect_json(RegionType::Void, "Void");
    expect_json(BodyType::ManifoldSolid, "ManifoldSolid");
    expect_json(BodyType::ManifoldSheet, "ManifoldSheet");
    expect_json(BodyType::ManifoldWire, "ManifoldWire");
    expect_json(BodyType::General, "General");
}

} // namespace
} // namespace prometheus::topology
