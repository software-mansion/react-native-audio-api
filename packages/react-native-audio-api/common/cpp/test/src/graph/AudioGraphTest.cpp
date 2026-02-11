#include <gtest/gtest.h>
#include <audioapi/core/utils/graph/AudioGraph.h>
#include <vector>
#include <algorithm>

namespace audioapi::utils::graph {

class AudioGraphTest : public ::testing::Test {
 protected:
  AudioGraph graph;
};

TEST_F(AudioGraphTest, CreateNode_AllocatesIndices) {
    uint32_t n1 = graph.createNode();
    uint32_t n2 = graph.createNode();
    uint32_t n3 = graph.createNode();

    EXPECT_EQ(n1, 0);
    EXPECT_EQ(n2, 1);
    EXPECT_EQ(n3, 2);

    EXPECT_EQ(graph.nodes.size(), 3);
}

TEST_F(AudioGraphTest, ReleaseNode_ReuseIndices) {
    uint32_t n0 = graph.createNode();
    uint32_t n1 = graph.createNode();
    uint32_t n2 = graph.createNode(); // size 3

    graph.releaseNode(n1); // release 1
    // Graph: [0:active, 1:free, 2:active]

    uint32_t n1_reuse = graph.createNode();
    EXPECT_EQ(n1_reuse, 1); // Should reuse 1

    uint32_t n3 = graph.createNode();
    EXPECT_EQ(n3, 3); // New slot
}

TEST_F(AudioGraphTest, ReleaseNode_LIFO_Logic) {
    // Current impl uses LIFO for free list (adds to head)
    uint32_t n0 = graph.createNode();
    uint32_t n1 = graph.createNode();
    uint32_t n2 = graph.createNode();

    graph.releaseNode(n0); // free list: 0
    graph.releaseNode(n2); // free list: 2 -> 0

    uint32_t r1 = graph.createNode();
    EXPECT_EQ(r1, 2); // Should get 2 (head)

    uint32_t r2 = graph.createNode();
    EXPECT_EQ(r2, 0); // Should get 0 (next)
}

TEST_F(AudioGraphTest, TopologicalSort_Simple) {
    uint32_t n0 = graph.createNode(); // 0
    uint32_t n1 = graph.createNode(); // 1
    uint32_t n2 = graph.createNode(); // 2

    // 0 -> 1 -> 2
    // Inputs: 1 needs 0, 2 needs 1
    // Outputs tracking removed from AudioGraph::Node
    graph.nodes[n1].inputs.push_back(n0);

    graph.nodes[n2].inputs.push_back(n1);

    graph.markDirty();
    graph.process();

    const auto& order = graph.executionOrder;
    EXPECT_EQ(order.size(), 3);

    // Verify order: 0 before 1, 1 before 2
    auto it0 = std::find(order.begin(), order.end(), n0);
    auto it1 = std::find(order.begin(), order.end(), n1);
    auto it2 = std::find(order.begin(), order.end(), n2);

    ASSERT_NE(it0, order.end());
    ASSERT_NE(it1, order.end());
    ASSERT_NE(it2, order.end());

    EXPECT_LT(std::distance(order.begin(), it0), std::distance(order.begin(), it1));
    EXPECT_LT(std::distance(order.begin(), it1), std::distance(order.begin(), it2));
}

TEST_F(AudioGraphTest, Process_SkipsFreeNodes) {
    uint32_t n0 = graph.createNode();
    uint32_t n1 = graph.createNode();

    graph.releaseNode(n0);

    graph.markDirty();
    graph.process();

    const auto& order = graph.executionOrder;
    // Should only contain n1
    EXPECT_EQ(order.size(), 1);
    EXPECT_EQ(order[0], n1);
}

} // namespace audioapi::utils::graph
