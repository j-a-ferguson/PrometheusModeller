#include "validate.hpp"

#include <gtest/gtest.h>
#include <format>
#include <set>

namespace prometheus::validate {
namespace {

Handle<Edge> add_edge_with_fins(Body &body, std::size_t count) {
    auto edge = body.addEdge();
    for (std::size_t i = 0; i < count; ++i) {
        auto fin = body.addFin();
        fin.get().edge = edge;
        EdgeFins::push_front(edge.get().fin, fin);
    }
    return edge;
}

TEST(Validate, NullEdgeDiagnosticOutlivesBody) {
    Result result;
    NodeId fin_id;
    {
        Modeller modeller;
        auto body = modeller.add_body();
        fin_id = body.get().addFin().get().id;
        result = validate(body);
    }

    ASSERT_EQ(result.errors.size(), 1u);
    const Diagnostic &error = result.errors.front();
    EXPECT_EQ(error.code, ErrorCode::EntityNull);
    EXPECT_EQ(error.node.name, "Fin");
    EXPECT_EQ(error.node.id, fin_id);
    EXPECT_EQ(error.member, "edge");
    EXPECT_EQ(error.message(), std::format("Fin {}.edge is null", fin_id));
}

TEST(Validate, StaleEdgeLinkReportsOwningFinBeforeEdgeCounts) {
    Modeller modeller;
    auto body = modeller.add_body();
    add_edge_with_fins(body.get(), 3);
    auto fin = body.get().addFin();
    SlotPool<Edge> edges;
    fin.get().edge = edges.emplace();
    ASSERT_TRUE(edges.erase(fin.get().edge));

    const Result result = validate(body);
    ASSERT_EQ(result.errors.size(), 1u);
    const Diagnostic &error = result.errors.front();
    EXPECT_EQ(error.code, ErrorCode::EntityStale);
    EXPECT_EQ(error.node.name, "Fin");
    EXPECT_EQ(error.node.id, fin.get().id);
    EXPECT_EQ(error.member, "edge");
    EXPECT_EQ(error.message(), std::format("Fin {}.edge is stale", fin.get().id));
}

TEST(Validate, EdgeWithTwoFinsPassesCountCheck) {
    Modeller modeller;
    auto body = modeller.add_body();
    add_edge_with_fins(body.get(), 2);

    const Result result = validate(body);
    EXPECT_TRUE(result.is_ok());
}

TEST(Validate, ReportsEachOverfullEdgeOnce) {
    Modeller modeller;
    auto body = modeller.add_body();
    std::set<NodeId> expected_ids{
        add_edge_with_fins(body.get(), 3).get().id,
        add_edge_with_fins(body.get(), 4).get().id
    };

    const Result result = validate(body);
    ASSERT_EQ(result.errors.size(), 2u);
    for (const Diagnostic &error : result.errors) {
        EXPECT_EQ(error.code, ErrorCode::EdgeTooManyFins);
        EXPECT_EQ(error.node.name, "Edge");
        EXPECT_EQ(expected_ids.erase(error.node.id), 1u);
        EXPECT_TRUE(error.member.empty());
        EXPECT_EQ(error.message(), std::format("Edge {} has more than 2 fins", error.node.id));
    }
    EXPECT_TRUE(expected_ids.empty());
}

} // namespace
} // namespace prometheus::validate
