#include <chains.hpp>
#include <schema.hpp>

#include <gtest/gtest.h>
#include <initializer_list>
#include <set>

namespace prometheus::topology {

TEST(Schema, BodyCreatesEachTopologyInItsOwnPool) {
    Modeller modeller;
    auto& body = modeller.add_body().get();

    auto check_pool = [&](auto add) {
        auto first = (body.*add)();
        auto second = (body.*add)();
        EXPECT_EQ(first.index(), 0u);
        EXPECT_EQ(second.index(), 1u);
        EXPECT_NE(&first.get(), &second.get());
    };

    check_pool(&Body::addVertex);
    check_pool(&Body::addEdge);
    check_pool(&Body::addFin);
    check_pool(&Body::addLoop);
    check_pool(&Body::addFace);
    check_pool(&Body::addShell);
    check_pool(&Body::addRegion);
}

TEST(Schema, ForEachNodeVisitsEveryBodyNodeOnce) {
    Modeller modeller;
    auto &body = modeller.add_body().get();
    std::set<NodeId> remaining{
        body.addVertex().get().id, body.addEdge().get().id, body.addFin().get().id,
        body.addLoop().get().id, body.addFace().get().id, body.addShell().get().id,
        body.addRegion().get().id, body.addPoint().get().id, body.addCurve().get().id,
        body.addSurface().get().id,
    };

    body.for_each_node([&](auto handle) {
        EXPECT_EQ(remaining.erase(handle.get().id), 1u);
    });
    EXPECT_TRUE(remaining.empty());
}

TEST(Schema, FinChainsCanBeEditedIndependently) {

    Modeller modeller;
    auto& body = modeller.add_body().get();
    auto& loop = body.addLoop().get();
    auto& vertex = body.addVertex().get();
    auto first = body.addFin();
    auto second = body.addFin();

    LoopFins::push_front(loop.fin, first);
    LoopFins::push_front(loop.fin, second);
    VertexFins::push_front(vertex.fin, second);
    VertexFins::push_front(vertex.fin, first);

    ASSERT_EQ(loop.fin, second);
    ASSERT_EQ(second.get().fin_next, first);
    ASSERT_EQ(second.get().fin_prev, first);
    ASSERT_EQ(first.get().fin_next, second);
    ASSERT_EQ(first.get().fin_prev, second);

    LoopFins::unlink(loop.fin, first);

    EXPECT_EQ(loop.fin, second);
    EXPECT_EQ(second.get().fin_next, second);
    EXPECT_EQ(second.get().fin_prev, second);
    EXPECT_TRUE(first.get().fin_next.is_null());
    EXPECT_TRUE(first.get().fin_prev.is_null());

    EXPECT_EQ(vertex.fin, first);
    EXPECT_EQ(first.get().fin_next_at_vertex, second);
    EXPECT_TRUE(second.get().fin_next_at_vertex.is_null());
}

TEST(Schema, CircularFinLoopHandlesSingleton) {

    Modeller modeller;
    auto& body = modeller.add_body().get();
    auto& loop = body.addLoop().get();
    auto fin = body.addFin();
    ASSERT_TRUE(loop.fin.is_null());

    LoopFins::push_front(loop.fin, fin);
    ASSERT_EQ(loop.fin, fin);
    ASSERT_EQ(fin.get().fin_next, fin);
    ASSERT_EQ(fin.get().fin_prev, fin);

    LoopFins::unlink(loop.fin, fin);
    EXPECT_TRUE(loop.fin.is_null());
    EXPECT_TRUE(fin.get().fin_next.is_null());
    EXPECT_TRUE(fin.get().fin_prev.is_null());
}

TEST(Schema, CircularFinLoopPreservesOrderAcrossRemoval) {
    Modeller modeller;
    auto& body = modeller.add_body().get();
    auto& loop = body.addLoop().get();
    auto first = body.addFin();
    auto middle = body.addFin();
    auto last = body.addFin();

    LoopFins::push_front(loop.fin, first);
    LoopFins::push_front(loop.fin, middle);
    LoopFins::push_front(loop.fin, last);

    ASSERT_EQ(loop.fin, last);
    ASSERT_EQ(last.get().fin_next, middle);
    ASSERT_EQ(middle.get().fin_next, first);
    ASSERT_EQ(first.get().fin_next, last);
    ASSERT_EQ(last.get().fin_prev, first);
    ASSERT_EQ(first.get().fin_prev, middle);
    ASSERT_EQ(middle.get().fin_prev, last);

    LoopFins::unlink(loop.fin, middle);
    ASSERT_EQ(loop.fin, last);
    ASSERT_EQ(last.get().fin_next, first);
    ASSERT_EQ(first.get().fin_next, last);
    ASSERT_EQ(last.get().fin_prev, first);
    ASSERT_EQ(first.get().fin_prev, last);
    EXPECT_TRUE(middle.get().fin_next.is_null());
    EXPECT_TRUE(middle.get().fin_prev.is_null());

    LoopFins::unlink(loop.fin, last);
    EXPECT_EQ(loop.fin, first);
    EXPECT_EQ(first.get().fin_next, first);
    EXPECT_EQ(first.get().fin_prev, first);
    EXPECT_TRUE(last.get().fin_next.is_null());
    EXPECT_TRUE(last.get().fin_prev.is_null());
}

TEST(Schema, LinearShellChainUnlinksAtEveryPosition) {
    Modeller modeller;
    auto& body = modeller.add_body().get();
    auto& region = body.addRegion().get();
    auto tail = body.addShell();
    auto survivor = body.addShell();
    auto middle = body.addShell();
    auto head = body.addShell();

    auto expect_chain = [&](std::initializer_list<Handle<Shell>> expected) {
        Handle<Shell> previous;
        auto current = region.shell;
        for (auto shell : expected) {
            ASSERT_EQ(current, shell);
            EXPECT_EQ(shell.get().shell_prev_in_region, previous);
            previous = shell;
            current = shell.get().shell_next_in_region;
        }
        EXPECT_TRUE(current.is_null());
    };

    auto unlink = [&](Handle<Shell> shell) {
        RegionShells::unlink(region.shell, shell);
        EXPECT_TRUE(shell.get().shell_prev_in_region.is_null());
        EXPECT_TRUE(shell.get().shell_next_in_region.is_null());
    };

    for (auto shell : {tail, survivor, middle, head})
        RegionShells::push_front(region.shell, shell);

    expect_chain({head, middle, survivor, tail});

    unlink(middle);
    expect_chain({head, survivor, tail});

    unlink(tail);
    expect_chain({head, survivor});

    unlink(head);
    expect_chain({survivor});

    unlink(survivor);
    expect_chain({});
}

TEST(Schema, SinglyLinkedUnlinkAtRewritesTheIncomingLink) {
    Modeller modeller;
    auto& body = modeller.add_body().get();
    auto& vertex = body.addVertex().get();
    auto tail = body.addFin();
    auto middle = body.addFin();
    auto head = body.addFin();

    VertexFins::push_front(vertex.fin, tail);
    VertexFins::push_front(vertex.fin, middle);
    VertexFins::push_front(vertex.fin, head);

    EXPECT_EQ(VertexFins::unlink_at(head.get().fin_next_at_vertex), middle);
    ASSERT_EQ(vertex.fin, head);
    ASSERT_EQ(head.get().fin_next_at_vertex, tail);
    EXPECT_TRUE(middle.get().fin_next_at_vertex.is_null());

    EXPECT_EQ(VertexFins::unlink_at(vertex.fin), head);
    ASSERT_EQ(vertex.fin, tail);
    EXPECT_TRUE(head.get().fin_next_at_vertex.is_null());

    ASSERT_TRUE(tail.get().fin_next_at_vertex.is_null());
    EXPECT_TRUE(VertexFins::unlink_at(tail.get().fin_next_at_vertex).is_null());
    EXPECT_EQ(vertex.fin, tail);
    EXPECT_TRUE(tail.get().fin_next_at_vertex.is_null());
}

TEST(Schema, SinglyLinkedUnlinkFindsOnlyMembers) {
    Modeller modeller;
    auto& body = modeller.add_body().get();
    auto& vertex = body.addVertex().get();
    auto tail = body.addFin();
    auto middle = body.addFin();
    auto head = body.addFin();

    VertexFins::push_front(vertex.fin, tail);
    VertexFins::push_front(vertex.fin, middle);
    VertexFins::push_front(vertex.fin, head);

    ASSERT_TRUE(VertexFins::unlink(vertex.fin, middle));
    ASSERT_EQ(vertex.fin, head);
    ASSERT_EQ(head.get().fin_next_at_vertex, tail);
    ASSERT_TRUE(tail.get().fin_next_at_vertex.is_null());
    EXPECT_TRUE(middle.get().fin_next_at_vertex.is_null());

    EXPECT_FALSE(VertexFins::unlink(vertex.fin, middle));
    EXPECT_EQ(vertex.fin, head);
    EXPECT_EQ(head.get().fin_next_at_vertex, tail);
    EXPECT_TRUE(tail.get().fin_next_at_vertex.is_null());
}

} // namespace prometheus::topology
