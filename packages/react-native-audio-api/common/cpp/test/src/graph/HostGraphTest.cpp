#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/core/utils/graph/AudioGraph.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <gtest/gtest.h>
#include <memory>
#include <utility>
#include <vector>
#include "TestGraphUtils.h"

namespace audioapi::utils::graph {

class HostGraphTest : public ::testing::Test {
 protected:
  static constexpr size_t kPayloadSize = audioapi::DISPOSER_PAYLOAD_SIZE;
  DisposerImpl<kPayloadSize> disposer_{64};

  void verifyAddEdge(
      HostGraph &hostGraph,
      AudioGraph &audioGraph,
      size_t fromId,
      size_t toId,
      const std::vector<std::vector<size_t>> &expectedAdjacencyList) {
    // Find nodes by ID

    HostGraph::Node *fromNode = nullptr;
    HostGraph::Node *toNode = nullptr;

    for (auto *n : hostGraph.nodes) {
      if (n->test_node_identifier__ == fromId)
        fromNode = n;
      if (n->test_node_identifier__ == toId)
        toNode = n;
    }

    ASSERT_NE(fromNode, nullptr);
    ASSERT_NE(toNode, nullptr);

    // Snapshot pre-state
    auto initialAudioAdj = TestGraphUtils::convertAudioGraphToAdjacencyList(audioGraph);

    // Action
    auto result = hostGraph.addEdge(fromNode, toNode);
    ASSERT_TRUE(result.is_ok()) << "addEdge failed";

    // Verify AudioGraph UNCHANGED
    auto intermediateAudioAdj = TestGraphUtils::convertAudioGraphToAdjacencyList(audioGraph);
    EXPECT_EQ(initialAudioAdj, intermediateAudioAdj) << "AudioGraph changed before event execution";

    // Perform Event
    auto event = std::move(result).unwrap();
    event(audioGraph, disposer_);

    // Verify AudioGraph UPDATED and CONSISTENT
    auto finalAudioAdj = TestGraphUtils::convertAudioGraphToAdjacencyList(audioGraph);
    auto finalHostAdj = TestGraphUtils::convertHostGraphToAdjacencyList(hostGraph);

    EXPECT_EQ(finalAudioAdj, expectedAdjacencyList)
        << "AudioGraph does not match expected adjacency list";
    EXPECT_EQ(finalHostAdj, expectedAdjacencyList)
        << "HostGraph does not match expected adjacency list";
  }

  HostGraph::Node *findNode(const HostGraph &hostGraph, size_t id) {
    for (auto *n : hostGraph.nodes) {
      if (n->test_node_identifier__ == id)
        return n;
    }
    return nullptr;
  }
};

TEST_F(HostGraphTest, AddNode) {
  auto [audioGraph, hostGraph] = TestGraphUtils::createTestGraph({
      {1, 2}, // 0 -> 1, 2
      {2},    // 1 -> 2
      {}      // 2
  });

  // Create a new handle and add it via HostGraph
  auto handle = std::make_shared<NodeHandle>(0, nullptr);
  auto [hostNode, event] = hostGraph.addNode(handle);

  EXPECT_EQ(hostNode->handle, handle);
  hostNode->test_node_identifier__ = 3;

  // AudioGraph unchanged before event
  EXPECT_EQ(audioGraph.size(), 3u);

  event(audioGraph, disposer_);

  // After event: node added to AudioGraph
  EXPECT_EQ(audioGraph.size(), 4u);
  audioGraph[handle->index].test_node_identifier__ = 3;

  auto adj = TestGraphUtils::convertAudioGraphToAdjacencyList(audioGraph);
  EXPECT_EQ(adj.size(), 4u);
}

TEST_F(HostGraphTest, AddEdge_SimpleForward) {
  // 0 -> 1   2
  // Add 1 -> 2
  auto [audioGraph, hostGraph] = TestGraphUtils::createTestGraph({
      {1}, // 0 -> 1
      {},  // 1
      {}   // 2
  });

  verifyAddEdge(
      hostGraph,
      audioGraph,
      1,
      2,
      {
          {1}, // 0 -> 1
          {2}, // 1 -> 2
          {}   // 2
      });
}

TEST_F(HostGraphTest, AddEdge_SimpleReorder) {
  // 0, 1 (Independent, assume 0 creates before 1)
  // Add 1 -> 0
  auto [audioGraph, hostGraph] = TestGraphUtils::createTestGraph({{}, {}});

  // Depending on implementation, making 1->0 implies 1 must be before 0.
  // Initial: 0, 1 (probably)
  // Expected: 1 -> 0

  verifyAddEdge(
      hostGraph,
      audioGraph,
      1,
      0,
      {
          {}, // 0
          {0} // 1 -> 0
      });
}

TEST_F(HostGraphTest, AddEdge_TransitiveReorder) {
  // 0 -> 1 -> 2, 3
  // initial: 0, 1, 2, 3 (3 is likely last or first? usually makes linear)
  // Add 2 -> 3
  // If 3 was before 0 (unlikely if created in order), or 3 was isolated.
  // Let's force a scenario where target is 'behind' source in list but valid topological wise.

  auto [audioGraph, hostGraph] = TestGraphUtils::createTestGraph({
      {1}, // 0->1
      {},  // 1
      {}   // 2
  });

  verifyAddEdge(hostGraph, audioGraph, 1, 2, {{1}, {2}, {}});
}

TEST_F(HostGraphTest, AddEdge_BackwardsLinkRequiringSort) {
  // 0 -> 1
  // 2
  // Initial order likely: 0, 1, 2. (or 2, 0, 1).
  // Add 2 -> 0.
  // Ensure 2 becomes before 0.
  auto [audioGraph, hostGraph] = TestGraphUtils::createTestGraph({{1}, {}, {}});

  verifyAddEdge(hostGraph, audioGraph, 2, 0, {{1}, {}, {0}});
}

TEST_F(HostGraphTest, AddEdge_diamond) {
  // 0, 1, 2, 3
  // 0->1, 0->2.
  // Add 1->3.
  // Add 2->3.
  auto [audioGraph, hostGraph] = TestGraphUtils::createTestGraph({{1, 2}, {}, {}, {}});

  verifyAddEdge(hostGraph, audioGraph, 1, 3, {{1, 2}, {3}, {}, {}});

  verifyAddEdge(hostGraph, audioGraph, 2, 3, {{1, 2}, {3}, {3}, {}});
}

TEST_F(HostGraphTest, AddEdge_MultiInputOutput) {
  // 0 -> 2
  // 1 -> 2
  // Add 2 -> 3
  auto [audioGraph, hostGraph] = TestGraphUtils::createTestGraph({{2}, {2}, {}, {}});

  verifyAddEdge(hostGraph, audioGraph, 2, 3, {{2}, {2}, {3}, {}});
}

TEST_F(HostGraphTest, AddEdge_CycleDetection) {
  // 0 -> 1 -> 2
  // Try to add 2 -> 0 (Cycle)
  auto [audioGraph, hostGraph] = TestGraphUtils::createTestGraph({
      {1}, // 0 -> 1
      {2}, // 1 -> 2
      {}   // 2
  });

  auto hostAdjBefore = TestGraphUtils::convertHostGraphToAdjacencyList(hostGraph);
  auto audioAdjBefore = TestGraphUtils::convertAudioGraphToAdjacencyList(audioGraph);

  HostGraph::Node *node0 = findNode(hostGraph, 0);
  HostGraph::Node *node2 = findNode(hostGraph, 2);

  // Try adding cycle 2->0
  auto result = hostGraph.addEdge(node2, node0);
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), HostGraph::ResultError::CYCLE_DETECTED);

  // HostGraph should NOT change
  auto hostAdjAfter = TestGraphUtils::convertHostGraphToAdjacencyList(hostGraph);
  EXPECT_EQ(hostAdjBefore, hostAdjAfter) << "HostGraph modified despite cycle detection";

  // AudioGraph should NOT change (no event executed)
  auto audioAdjAfter = TestGraphUtils::convertAudioGraphToAdjacencyList(audioGraph);
  EXPECT_EQ(audioAdjBefore, audioAdjAfter) << "AudioGraph modified";
}

TEST_F(HostGraphTest, AddEdge_LargeSpecificGraph) {
  // Create two separate chains and link them
  // Chain 1: 0->1->2->3->4
  // Chain 2: 5->6->7->8->9
  // Add 4->5

  std::vector<std::vector<size_t>> adj(10);
  for (int i = 0; i < 4; ++i)
    adj[i] = {static_cast<size_t>(i + 1)};
  adj[4] = {};
  for (int i = 5; i < 9; ++i)
    adj[i] = {static_cast<size_t>(i + 1)};
  adj[9] = {};

  auto [audioGraph, hostGraph] = TestGraphUtils::createTestGraph(adj);

  std::vector<std::vector<size_t>> expectedAdj = adj;
  expectedAdj[4].push_back(5);

  verifyAddEdge(hostGraph, audioGraph, 4, 5, expectedAdj);
}

TEST_F(HostGraphTest, AddEdge_GridInterconnect) {
  // 0  1  2
  // |  |  |
  // 3  4  5
  //
  // Connections: 0->3, 1->4, 2->5
  // Add 3->4 (valid)
  // Add 4->2 (valid, requires 4 before 2? No, 1->4->2... so 1 before 2)

  std::vector<std::vector<size_t>> adj(6);
  adj[0] = {3};
  adj[1] = {4};
  adj[2] = {5};

  auto [audioGraph, hostGraph] = TestGraphUtils::createTestGraph(adj);

  auto expected = adj;
  expected[3].push_back(4);
  verifyAddEdge(hostGraph, audioGraph, 3, 4, expected);

  expected[4].push_back(2);
  verifyAddEdge(hostGraph, audioGraph, 4, 2, expected);

  // Now we have path 0->3->4->2->5
  // And 1->4->2->5
  // If we try 5->0 -> Cycle (5 reachable from 0)

  auto hostAdjBefore = TestGraphUtils::convertHostGraphToAdjacencyList(hostGraph);
  HostGraph::Node *node5 = findNode(hostGraph, 5);
  HostGraph::Node *node0 = findNode(hostGraph, 0);

  auto result = hostGraph.addEdge(node5, node0);
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(hostAdjBefore, TestGraphUtils::convertHostGraphToAdjacencyList(hostGraph));
}

// ---------------------------------------------------------------------------
// BUG demonstration: ghost node in AudioGraph causes accepted cycle
// ---------------------------------------------------------------------------
//
// When a node is removed from HostGraph it is deleted immediately (edges torn
// down, pointer freed).  The corresponding AudioGraph event only marks the
// node as `orphaned` — it stays in the vector with all its edges until
// compaction eventually removes it.
//
// This creates a window where HostGraph no longer "sees" the node, so its
// cycle-detection (hasPath) can miss paths that still exist in AudioGraph.
// If a new edge is added through that blind-spot, AudioGraph ends up with a
// cycle and toposort produces garbage.
//
TEST_F(HostGraphTest, RemoveNode_GhostNodeMustNotAllowCycle) {
  // Setup: chain  0 → 1 → 2
  auto [audioGraph, hostGraph] = TestGraphUtils::createTestGraph({
      {1}, // 0 -> 1
      {2}, // 1 -> 2
      {}   // 2
  });

  HostGraph::Node *node0 = findNode(hostGraph, 0);
  HostGraph::Node *node1 = findNode(hostGraph, 1);
  HostGraph::Node *node2 = findNode(hostGraph, 2);
  ASSERT_NE(node0, nullptr);
  ASSERT_NE(node1, nullptr);
  ASSERT_NE(node2, nullptr);

  // ── Step 1: remove node 1 from HostGraph ──
  auto removeResult = hostGraph.removeNode(node1);
  ASSERT_TRUE(removeResult.is_ok());

  // Execute the remove-event on AudioGraph (only sets orphaned=true).
  auto removeEvent = std::move(removeResult).unwrap();
  removeEvent(audioGraph, disposer_);

  // AudioGraph still has the ghost: 0 → 1(orphaned) → 2
  EXPECT_EQ(audioGraph.size(), 3u);

  // ── Step 2: add edge 2 → 0 ──
  // Because node 1 still bridges 0→2 in AudioGraph, this would create
  // a cycle: 0 → 1 → 2 → 0.  HostGraph MUST reject it.
  auto addResult = hostGraph.addEdge(node2, node0);

  EXPECT_TRUE(addResult.is_err()) << "HostGraph should detect the cycle through the ghost node";
  EXPECT_EQ(addResult.unwrap_err(), HostGraph::ResultError::CYCLE_DETECTED);
}

} // namespace audioapi::utils::graph
