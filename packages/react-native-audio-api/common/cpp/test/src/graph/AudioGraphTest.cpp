#include <gtest/gtest.h>
#include <audioapi/core/utils/graph/AudioGraph.hpp>
#include <audioapi/core/utils/graph/NodeHandle.hpp>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/utils/AudioBus.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <vector>
#include <algorithm>
#include <utility>
#include <memory>

using namespace audioapi;

namespace audioapi::utils::graph {

// ---------------------------------------------------------------------------
// MockAudioNode — lightweight AudioNode subclass for testing canBeDestructed
// ---------------------------------------------------------------------------
class MockAudioNode : public AudioNode {
 public:
  explicit MockAudioNode(
      const std::shared_ptr<BaseAudioContext> &ctx,
      bool destructible = true)
      : AudioNode(ctx), destructible_(destructible) {}

  bool canBeDestructed() const override {
    return destructible_;
  }

  void setDestructible(bool v) {
    destructible_ = v;
  }

 protected:
  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus> &, int) override {
    return nullptr;
  }

 private:
  bool destructible_;
};

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------
class AudioGraphTest : public ::testing::Test {
 protected:
  AudioGraph<AudioNode> graph;

  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> ctx;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    ctx = std::make_shared<OfflineAudioContext>(
        2, 44100 * 5, 44100, eventRegistry, RuntimeRegistry{});
  }

  // Helpers ----------------------------------------------------------------

  /// @brief Creates a shared NodeHandle<AudioNode> with no AudioNode (for structural tests)
  std::shared_ptr<NodeHandle<AudioNode>> makeHandle(size_t testId = 0) {
    auto h = std::make_shared<NodeHandle<AudioNode>>(0, nullptr);
    return h;
  }

  /// @brief Creates a shared NodeHandle<AudioNode> with a MockAudioNode
  std::shared_ptr<NodeHandle<AudioNode>> makeHandleWithNode(bool destructible = true) {
    auto node = std::make_unique<MockAudioNode>(ctx, destructible);
    return std::make_shared<NodeHandle<AudioNode>>(0, std::move(node));
  }

  /// @brief Adds N nodes with test identifiers 0..N-1 and returns their handles
  std::vector<std::shared_ptr<NodeHandle<AudioNode>>> addNodes(size_t n, bool withAudioNode = false) {
    std::vector<std::shared_ptr<NodeHandle<AudioNode>>> handles;
    handles.reserve(n);
    for (size_t i = 0; i < n; i++) {
      auto h = withAudioNode ? makeHandleWithNode() : makeHandle(i);
      graph.addNode(h);
      graph[h->index].test_node_identifier__ = i;
      handles.push_back(std::move(h));
    }
    return handles;
  }

  /// @brief Returns the test_node_identifier__ sequence after toposort
  std::vector<size_t> getOrder() {
    std::vector<size_t> order;
    order.reserve(graph.size());
    for (uint32_t i = 0; i < graph.size(); i++) {
      order.push_back(graph[i].test_node_identifier__);
    }
    return order;
  }

  /// @brief Returns position of testId in the current order
  int posOf(size_t testId) {
    for (uint32_t i = 0; i < graph.size(); i++) {
      if (graph[i].test_node_identifier__ == testId) return static_cast<int>(i);
    }
    return -1;
  }
};

// =====================================================================
// addNode
// =====================================================================

TEST_F(AudioGraphTest, AddNode_AssignsSequentialIndices) {
  auto h0 = makeHandle();
  auto h1 = makeHandle();
  auto h2 = makeHandle();
  graph.addNode(h0);
  graph.addNode(h1);
  graph.addNode(h2);

  EXPECT_EQ(h0->index, 0u);
  EXPECT_EQ(h1->index, 1u);
  EXPECT_EQ(h2->index, 2u);
  EXPECT_EQ(graph.size(), 3u);
}

TEST_F(AudioGraphTest, AddNode_EmptyGraph) {
  EXPECT_TRUE(graph.empty());
  EXPECT_EQ(graph.size(), 0u);
}

// =====================================================================
// Topological sort — simple chain
// =====================================================================

TEST_F(AudioGraphTest, TopoSort_LinearChain) {
  // 0 -> 1 -> 2  (inputs: 1 has 0, 2 has 1)
  auto h = addNodes(3);
  graph[h[1]->index].inputs.push_back(h[0]->index);
  graph[h[2]->index].inputs.push_back(h[1]->index);

  graph.markDirty();
  graph.process();

  EXPECT_LT(posOf(0), posOf(1));
  EXPECT_LT(posOf(1), posOf(2));
}

TEST_F(AudioGraphTest, TopoSort_ReversedInsertion) {
  // Insert in reverse but edges say 0 -> 1 -> 2
  auto h = addNodes(3);
  // Wire: 2 needs 1, 1 needs 0
  graph[h[2]->index].inputs.push_back(h[1]->index);
  graph[h[1]->index].inputs.push_back(h[0]->index);

  graph.markDirty();
  graph.process();

  EXPECT_LT(posOf(0), posOf(1));
  EXPECT_LT(posOf(1), posOf(2));
}

// =====================================================================
// Topological sort — diamond
// =====================================================================

TEST_F(AudioGraphTest, TopoSort_Diamond) {
  //     0
  //    / \
  //   1   2
  //    \ /
  //     3
  auto h = addNodes(4);
  graph[h[1]->index].inputs.push_back(h[0]->index);
  graph[h[2]->index].inputs.push_back(h[0]->index);
  graph[h[3]->index].inputs.push_back(h[1]->index);
  graph[h[3]->index].inputs.push_back(h[2]->index);

  graph.markDirty();
  graph.process();

  EXPECT_LT(posOf(0), posOf(1));
  EXPECT_LT(posOf(0), posOf(2));
  EXPECT_LT(posOf(1), posOf(3));
  EXPECT_LT(posOf(2), posOf(3));
}

// =====================================================================
// Topological sort — multi-input fan-in
// =====================================================================

TEST_F(AudioGraphTest, TopoSort_FanIn) {
  // 0, 1, 2 all feed into 3
  auto h = addNodes(4);
  graph[h[3]->index].inputs.push_back(h[0]->index);
  graph[h[3]->index].inputs.push_back(h[1]->index);
  graph[h[3]->index].inputs.push_back(h[2]->index);

  graph.markDirty();
  graph.process();

  EXPECT_LT(posOf(0), posOf(3));
  EXPECT_LT(posOf(1), posOf(3));
  EXPECT_LT(posOf(2), posOf(3));
}

// =====================================================================
// Topological sort — disconnected components
// =====================================================================

TEST_F(AudioGraphTest, TopoSort_DisconnectedComponents) {
  // Two separate chains: 0->1   2->3
  auto h = addNodes(4);
  graph[h[1]->index].inputs.push_back(h[0]->index);
  graph[h[3]->index].inputs.push_back(h[2]->index);

  graph.markDirty();
  graph.process();

  EXPECT_LT(posOf(0), posOf(1));
  EXPECT_LT(posOf(2), posOf(3));
}

// =====================================================================
// Topological sort — single node (no edges)
// =====================================================================

TEST_F(AudioGraphTest, TopoSort_SingleNode) {
  auto h = addNodes(1);

  graph.markDirty();
  graph.process();

  EXPECT_EQ(graph.size(), 1u);
  EXPECT_EQ(posOf(0), 0);
}

// =====================================================================
// Topological sort — no-op when not dirty
// =====================================================================

TEST_F(AudioGraphTest, TopoSort_SkippedWhenNotDirty) {
  auto h = addNodes(3);
  graph[h[1]->index].inputs.push_back(h[0]->index);
  graph[h[2]->index].inputs.push_back(h[1]->index);

  graph.markDirty();
  graph.process();

  auto orderAfterFirst = getOrder();

  // process again without marking dirty — order should stay identical
  graph.process();

  EXPECT_EQ(getOrder(), orderAfterFirst);
}

// =====================================================================
// Compaction — orphaned leaf with canBeDestructed=true
// =====================================================================

TEST_F(AudioGraphTest, Compact_RemovesOrphanedDestructibleLeaf) {
  // 3 nodes: 0 -> 1 -> 2, mark node 2 as orphaned
  auto h = addNodes(3, /*withAudioNode=*/true);
  graph[h[1]->index].inputs.push_back(h[0]->index);
  graph[h[2]->index].inputs.push_back(h[1]->index);
  graph[h[2]->index].orphaned = true;

  // also clear node 2's inputs so it qualifies (orphaned + no inputs + canBeDestructed)
  graph[h[2]->index].inputs.clear();

  graph.markDirty();
  graph.process();

  EXPECT_EQ(graph.size(), 2u);
  // Node 2 should be gone
  EXPECT_EQ(posOf(2), -1);
}

// =====================================================================
// Compaction — orphaned node with inputs is NOT removed
// =====================================================================

TEST_F(AudioGraphTest, Compact_KeepsOrphanedNodeWithInputs) {
  auto h = addNodes(2, /*withAudioNode=*/true);
  graph[h[1]->index].inputs.push_back(h[0]->index);
  graph[h[1]->index].orphaned = true;

  graph.markDirty();
  graph.process();

  // Node 1 is orphaned but still has inputs — should stay
  EXPECT_EQ(graph.size(), 2u);
}

// =====================================================================
// Compaction — orphaned node with canBeDestructed=false is NOT removed
// =====================================================================

TEST_F(AudioGraphTest, Compact_KeepsNonDestructible) {
  auto h0 = makeHandleWithNode(/*destructible=*/true);
  auto h1 = makeHandleWithNode(/*destructible=*/false); // can't be destructed

  graph.addNode(h0);
  graph[h0->index].test_node_identifier__ = 0;
  graph.addNode(h1);
  graph[h1->index].test_node_identifier__ = 1;

  graph[h1->index].orphaned = true;
  // h1 has no inputs, is orphaned, but canBeDestructed() returns false

  graph.markDirty();
  graph.process();

  EXPECT_EQ(graph.size(), 2u); // still 2 — node 1 stays
}

// =====================================================================
// Compaction — canBeDestructed becomes true later
// =====================================================================

TEST_F(AudioGraphTest, Compact_RemovesOnceDestructible) {
  auto h0 = makeHandleWithNode(/*destructible=*/true);
  auto h1 = makeHandleWithNode(/*destructible=*/false);

  graph.addNode(h0);
  graph[h0->index].test_node_identifier__ = 0;
  graph.addNode(h1);
  graph[h1->index].test_node_identifier__ = 1;

  graph[h1->index].orphaned = true;

  graph.markDirty();
  graph.process(); // first pass: node 1 stays (not destructible)
  EXPECT_EQ(graph.size(), 2u);

  // Now make it destructible
  static_cast<MockAudioNode *>(h1->audioNode.get())->setDestructible(true);

  graph.process(); // second pass: node 1 should be removed
  EXPECT_EQ(graph.size(), 1u);
  EXPECT_EQ(posOf(1), -1);
}

// =====================================================================
// Compaction — handle indices are updated correctly
// =====================================================================

TEST_F(AudioGraphTest, Compact_UpdatesHandleIndices) {
  auto h = addNodes(4, /*withAudioNode=*/true);
  // Chain: 0 -> 1 -> 2 -> 3
  graph[h[1]->index].inputs.push_back(h[0]->index);
  graph[h[2]->index].inputs.push_back(h[1]->index);
  graph[h[3]->index].inputs.push_back(h[2]->index);

  graph.markDirty();
  graph.process();

  // Now orphan node 1 (remove its inputs so it can be deleted)
  graph[h[1]->index].orphaned = true;
  graph[h[1]->index].inputs.clear();

  // Also remove node 1 from node 2's inputs (otherwise it references a deleted node)
  auto &n2inputs = graph[h[2]->index].inputs;
  n2inputs.erase(
      std::remove(n2inputs.begin(), n2inputs.end(), h[1]->index),
      n2inputs.end());

  graph.process();

  // After compaction: 3 nodes remain (0, 2, 3)
  EXPECT_EQ(graph.size(), 3u);

  // Each surviving handle's index should match its actual position
  for (uint32_t i = 0; i < graph.size(); i++) {
    EXPECT_EQ(graph[i].handle->index, i);
  }
}

// =====================================================================
// Compaction — multiple orphaned nodes at once
// =====================================================================

TEST_F(AudioGraphTest, Compact_MultipleOrphans) {
  auto h = addNodes(5, /*withAudioNode=*/true);
  // No edges — all independent
  graph[h[1]->index].orphaned = true;
  graph[h[3]->index].orphaned = true;

  graph.markDirty();
  graph.process();

  EXPECT_EQ(graph.size(), 3u);
  EXPECT_NE(posOf(0), -1);
  EXPECT_EQ(posOf(1), -1);
  EXPECT_NE(posOf(2), -1);
  EXPECT_EQ(posOf(3), -1);
  EXPECT_NE(posOf(4), -1);

  // Verify handle indices
  for (uint32_t i = 0; i < graph.size(); i++) {
    EXPECT_EQ(graph[i].handle->index, i);
  }
}

// =====================================================================
// Compaction — cascading removal (input removed causes downstream to become leaf)
// =====================================================================

TEST_F(AudioGraphTest, Compact_CascadingRemoval) {
  // 0 -> 1 (orphaned), node 1 has no other inputs
  // After removing 1, no further cascade because 0 is not orphaned
  auto h = addNodes(2, /*withAudioNode=*/true);
  graph[h[1]->index].inputs.push_back(h[0]->index);

  // Orphan node 0 — it has no inputs, so it's removable
  graph[h[0]->index].orphaned = true;

  graph.markDirty();
  graph.process();

  // Node 0 should be removed (orphaned, no inputs, destructible)
  // Node 1 still references old index of 0 in its inputs — that input will be cleaned on next process()
  EXPECT_EQ(graph.size(), 1u);
}

// =====================================================================
// Compaction — empty graph after removing all nodes
// =====================================================================

TEST_F(AudioGraphTest, Compact_RemoveAllNodes) {
  auto h = addNodes(3, /*withAudioNode=*/true);
  graph[h[0]->index].orphaned = true;
  graph[h[1]->index].orphaned = true;
  graph[h[2]->index].orphaned = true;

  graph.markDirty();
  graph.process();

  EXPECT_EQ(graph.size(), 0u);
  EXPECT_TRUE(graph.empty());
}

// =====================================================================
// Process on empty graph — no crash
// =====================================================================

TEST_F(AudioGraphTest, Process_EmptyGraph) {
  graph.process();
  EXPECT_EQ(graph.size(), 0u);
}

// =====================================================================
// MarkDirty — multiple calls are idempotent
// =====================================================================

TEST_F(AudioGraphTest, MarkDirty_Idempotent) {
  auto h = addNodes(2);
  graph[h[1]->index].inputs.push_back(h[0]->index);

  graph.markDirty();
  graph.markDirty();
  graph.markDirty();
  graph.process();

  EXPECT_EQ(graph.size(), 2u);
  EXPECT_LT(posOf(0), posOf(1));
}

// =====================================================================
// Toposort — complex multi-layer DAG
// =====================================================================

TEST_F(AudioGraphTest, TopoSort_ComplexDAG) {
  //   0    1
  //   |\ / |
  //   | 2  |
  //   |/ \ |
  //   3   4
  //    \ /
  //     5
  auto h = addNodes(6);
  graph[h[2]->index].inputs.push_back(h[0]->index);
  graph[h[2]->index].inputs.push_back(h[1]->index);
  graph[h[3]->index].inputs.push_back(h[0]->index);
  graph[h[3]->index].inputs.push_back(h[2]->index);
  graph[h[4]->index].inputs.push_back(h[2]->index);
  graph[h[4]->index].inputs.push_back(h[1]->index);
  graph[h[5]->index].inputs.push_back(h[3]->index);
  graph[h[5]->index].inputs.push_back(h[4]->index);

  graph.markDirty();
  graph.process();

  // Check all dependency constraints
  EXPECT_LT(posOf(0), posOf(2));
  EXPECT_LT(posOf(1), posOf(2));
  EXPECT_LT(posOf(0), posOf(3));
  EXPECT_LT(posOf(2), posOf(3));
  EXPECT_LT(posOf(2), posOf(4));
  EXPECT_LT(posOf(1), posOf(4));
  EXPECT_LT(posOf(3), posOf(5));
  EXPECT_LT(posOf(4), posOf(5));
}

// =====================================================================
// Successive process calls — interleaved add & compact
// =====================================================================

TEST_F(AudioGraphTest, Process_AddAndRemoveInterleaved) {
  auto h0 = makeHandleWithNode();
  auto h1 = makeHandleWithNode();
  graph.addNode(h0);
  graph[h0->index].test_node_identifier__ = 0;
  graph.addNode(h1);
  graph[h1->index].test_node_identifier__ = 1;

  graph.markDirty();
  graph.process();
  EXPECT_EQ(graph.size(), 2u);

  // Orphan 0, add 2
  graph[h0->index].orphaned = true;
  auto h2 = makeHandleWithNode();
  graph.addNode(h2);
  graph[h2->index].test_node_identifier__ = 2;

  graph.markDirty();
  graph.process();

  EXPECT_EQ(graph.size(), 2u);
  EXPECT_EQ(posOf(0), -1);
  EXPECT_NE(posOf(1), -1);
  EXPECT_NE(posOf(2), -1);
}

} // namespace audioapi::utils::graph
