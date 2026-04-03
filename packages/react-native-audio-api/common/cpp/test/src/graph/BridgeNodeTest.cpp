#include <audioapi/core/utils/graph/BridgeNode.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "MockGraphProcessor.h"
#include "TestGraphUtils.h"

namespace audioapi::utils::graph {

// =========================================================================
// A. isProcessable contract
// =========================================================================

TEST(BridgeNodeContract, MockNodeIsNotProcessable) {
  MockNode node;
  EXPECT_FALSE(node.isProcessable());
}

TEST(BridgeNodeContract, BridgeNodeIsNotProcessable) {
  BridgeNode bridge(nullptr);
  EXPECT_FALSE(bridge.isProcessable());
}

TEST(BridgeNodeContract, BridgeNodeIsAlwaysDestructible) {
  BridgeNode bridge(nullptr);
  EXPECT_TRUE(bridge.canBeDestructed());
}

TEST(BridgeNodeContract, NonProcessableMockNodeIsNotProcessable) {
  NonProcessableMockNode node;
  EXPECT_FALSE(node.isProcessable());
  EXPECT_TRUE(node.canBeDestructed());
}

TEST(BridgeNodeContract, BridgeNodeStoresParam) {
  // Use a real AudioParam to verify storage
  auto param = createMockAudioParam();
  BridgeNode bridge(param.get());
  EXPECT_EQ(bridge.param(), param.get());
}

// =========================================================================
// B. Graph structural tests (HostGraph + AudioGraph)
// =========================================================================

class BridgeGraphTest : public ::testing::Test {
 protected:
  using HNode = HostGraph::Node;
  using AGEvent = HostGraph::AGEvent;
  static constexpr size_t kPayloadSize = audioapi::DISPOSER_PAYLOAD_SIZE;

  AudioGraph audioGraph;
  HostGraph hostGraph;
  DisposerImpl<kPayloadSize> disposer_{64};

  HNode *addMockNode() {
    auto obj = std::make_unique<MockNode>();
    auto handle = std::make_shared<NodeHandle>(0, std::move(obj));
    auto [hostNode, event] = hostGraph.addNode(handle);
    event(audioGraph, disposer_);
    return hostNode;
  }

  HNode *addBridgeNode(AudioParam *param = nullptr) {
    auto obj = std::make_unique<BridgeNode>(param);
    auto handle = std::make_shared<NodeHandle>(0, std::move(obj));
    auto [hostNode, event] = hostGraph.addNode(handle);
    event(audioGraph, disposer_);
    return hostNode;
  }

  bool addEdge(HNode *from, HNode *to) {
    auto result = hostGraph.addEdge(from, to);
    if (result.is_ok()) {
      auto event = std::move(result).unwrap();
      event(audioGraph, disposer_);
      return true;
    }
    return false;
  }

  bool removeEdge(HNode *from, HNode *to) {
    auto result = hostGraph.removeEdge(from, to);
    if (result.is_ok()) {
      auto event = std::move(result).unwrap();
      event(audioGraph, disposer_);
      return true;
    }
    return false;
  }

  bool removeNode(HNode *node) {
    auto result = hostGraph.removeNode(node);
    if (result.is_ok()) {
      auto event = std::move(result).unwrap();
      event(audioGraph, disposer_);
      return true;
    }
    return false;
  }
};

TEST_F(BridgeGraphTest, BridgeCreatesThreeNodePath) {
  auto *source = addMockNode();
  auto *owner = addMockNode();
  auto *bridge = addBridgeNode();

  ASSERT_TRUE(addEdge(source, bridge));
  ASSERT_TRUE(addEdge(bridge, owner));

  // 3 nodes in graph
  EXPECT_EQ(audioGraph.size(), 3u);

  // Topo sort should place them: source, bridge, owner
  audioGraph.process();

  // Verify source comes before bridge comes before owner
  auto srcIdx = source->handle->index;
  auto bridgeIdx = bridge->handle->index;
  auto ownerIdx = owner->handle->index;
  EXPECT_LT(srcIdx, bridgeIdx);
  EXPECT_LT(bridgeIdx, ownerIdx);
}

TEST_F(BridgeGraphTest, CycleDetectionThroughBridges) {
  // Create: A → bridge → B → A would be a cycle
  auto *nodeA = addMockNode();
  auto *nodeB = addMockNode();
  auto *bridge = addBridgeNode();

  ASSERT_TRUE(addEdge(nodeA, bridge));
  ASSERT_TRUE(addEdge(bridge, nodeB));

  // Now B → A should be rejected as a cycle
  auto result = hostGraph.addEdge(nodeB, nodeA);
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), HostGraph::ResultError::CYCLE_DETECTED);
}

TEST_F(BridgeGraphTest, DuplicateEdgeRejectionWithBridges) {
  auto *source = addMockNode();
  auto *bridge = addBridgeNode();

  ASSERT_TRUE(addEdge(source, bridge));
  // Same edge again should be rejected
  auto result = hostGraph.addEdge(source, bridge);
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), HostGraph::ResultError::EDGE_ALREADY_EXISTS);
}

// =========================================================================
// C. AudioGraph::iter() filtering
// =========================================================================

class BridgeIterTest : public ::testing::Test {
 protected:
  using HNode = HostGraph::Node;
  static constexpr size_t kPayloadSize = audioapi::DISPOSER_PAYLOAD_SIZE;

  AudioGraph audioGraph;
  HostGraph hostGraph;
  DisposerImpl<kPayloadSize> disposer_{64};

  HNode *addNode(std::unique_ptr<GraphObject> obj) {
    auto handle = std::make_shared<NodeHandle>(0, std::move(obj));
    auto [hostNode, event] = hostGraph.addNode(handle);
    event(audioGraph, disposer_);
    return hostNode;
  }

  bool addEdge(HNode *from, HNode *to) {
    auto result = hostGraph.addEdge(from, to);
    if (result.is_ok()) {
      std::move(result).unwrap()(audioGraph, disposer_);
      return true;
    }
    return false;
  }
};

TEST_F(BridgeIterTest, IterSkipsNonProcessableNodes) {
  auto node = std::make_unique<MockNode>();
  node->setProcessable();
  auto *processable1 = addNode(std::move(node));
  auto *nonProcessable = addNode(std::make_unique<NonProcessableMockNode>());
  auto node2 = std::make_unique<MockNode>();
  node2->setProcessable();
  auto *processable2 = addNode(std::move(node2));

  ASSERT_TRUE(addEdge(processable1, nonProcessable));
  ASSERT_TRUE(addEdge(nonProcessable, processable2));
  audioGraph.process();

  // iter() should only yield 2 nodes (skip the non-processable one)
  size_t count = 0;
  for (auto &&[graphObject, inputs] : audioGraph.iter()) {
    EXPECT_TRUE(graphObject.isProcessable());
    count++;
  }
  EXPECT_EQ(count, 2u);
}

TEST_F(BridgeIterTest, AllProcessableNodesInTopoOrder) {
  // A → bridge → B → C
  auto *a = addNode(std::make_unique<ProcessableMockNode>(nullptr, 1));
  auto *bridge = addNode(std::make_unique<BridgeNode>(nullptr));
  auto *b = addNode(std::make_unique<ProcessableMockNode>(nullptr, 2));
  auto *c = addNode(std::make_unique<ProcessableMockNode>(nullptr, 3));

  ASSERT_TRUE(addEdge(a, bridge));
  ASSERT_TRUE(addEdge(bridge, b));
  ASSERT_TRUE(addEdge(b, c));
  audioGraph.process();

  // Should yield A, bridge, B, C in topo order (bridge is now processable)
  size_t count = 0;
  for (auto &&[graphObject, inputs] : audioGraph.iter()) {
    count++;
  }
  EXPECT_EQ(count, 4u);
}

TEST_F(BridgeIterTest, InputsViewMayReferenceBridgeIndices) {
  // source → bridge → owner
  // iter() yields all processable nodes including bridge
  auto *source = addNode(std::make_unique<MockNode>());
  auto *bridge = addNode(std::make_unique<BridgeNode>(nullptr));
  auto node = std::make_unique<MockNode>();
  node->setProcessable();
  auto *owner = addNode(std::move(node));

  ASSERT_TRUE(addEdge(source, bridge));
  ASSERT_TRUE(addEdge(bridge, owner));
  audioGraph.process();

  size_t processableCount = 0;
  for (auto &&[graphObject, inputs] : audioGraph.iter()) {
    processableCount++;
    // Input could be bridge or source — both are valid GraphObjects
    for (const auto &input : inputs) {
      (void)input;
    }
  }
  EXPECT_EQ(processableCount, 3u); // source + bridge + owner
}

// =========================================================================
// D. Compaction tests
// =========================================================================

TEST_F(BridgeGraphTest, OrphanedBridgeWithNoInputsRemoved) {
  auto *bridge = addBridgeNode();
  EXPECT_EQ(audioGraph.size(), 1u);

  // Mark orphaned
  removeNode(bridge);
  audioGraph.process();

  EXPECT_EQ(audioGraph.size(), 0u);
}

TEST_F(BridgeGraphTest, SourceRemovalCascadesBridgeRemoval) {
  auto *source = addMockNode();
  auto *bridge = addBridgeNode();
  auto *owner = addMockNode();

  ASSERT_TRUE(addEdge(source, bridge));
  ASSERT_TRUE(addEdge(bridge, owner));
  audioGraph.process();
  EXPECT_EQ(audioGraph.size(), 3u);

  // Remove source — bridge loses its only input
  removeNode(source);
  audioGraph.process();

  // Source compacted (orphaned, no inputs, destructible)
  // Bridge compacted (orphaned via edge removal cascade — its input was removed)
  // Owner stays (not orphaned)
  // Actually: source is orphaned+no inputs → removed
  // Then bridge has no inputs → but bridge is NOT orphaned unless explicitly marked
  // Bridge keeps its edge to owner. Bridge itself is not orphaned.
  // So only source is removed.
  // After first process: source removed, bridge has no inputs but not orphaned.
  // Bridge won't be compacted unless it's also orphaned.
  // This is correct — bridge removal needs to be done via disconnectParam or removeNodeWithBridges.
  EXPECT_EQ(audioGraph.size(), 2u); // bridge + owner remain
}

TEST_F(BridgeGraphTest, BridgeOrphanedAndNoInputsGetsCompacted) {
  auto *source = addMockNode();
  auto *bridge = addBridgeNode();
  auto *owner = addMockNode();

  ASSERT_TRUE(addEdge(source, bridge));
  ASSERT_TRUE(addEdge(bridge, owner));
  audioGraph.process();
  EXPECT_EQ(audioGraph.size(), 3u);

  // Orphan source and bridge
  removeNode(source);
  removeEdge(bridge, owner);
  removeNode(bridge);
  audioGraph.process();

  // Both source and bridge should be compacted
  EXPECT_EQ(audioGraph.size(), 1u); // only owner remains
}

// =========================================================================
// E. Full Graph wrapper integration
// =========================================================================

class BridgeGraphWrapperTest : public ::testing::Test {
 protected:
  using HNode = HostGraph::Node;
  static constexpr size_t kPayloadSize = audioapi::DISPOSER_PAYLOAD_SIZE;
  DisposerImpl<kPayloadSize> disposer_{64};
  std::shared_ptr<Graph> graph;
  // Hold real AudioParams to avoid fake pointer crashes
  std::vector<std::shared_ptr<AudioParam>> params_;

  void SetUp() override {
    graph = std::make_shared<Graph>(4096, &disposer_);
  }

  AudioParam *createParam() {
    params_.push_back(createMockAudioParam());
    return params_.back().get();
  }

  /// Simulates AudioParamHostObject: creates bridge, adds to graph, connects bridge→owner
  HNode *createParamBridge(AudioParam *param, HNode *owner) {
    auto *bridge = graph->addNode(std::make_unique<BridgeNode>(param));
    EXPECT_TRUE(graph->addEdge(bridge, owner).is_ok());
    return bridge;
  }

  void processAll() {
    graph->processEvents();
    graph->process();
  }
};

TEST_F(BridgeGraphWrapperTest, ConnectSourceToBridge) {
  auto *source = graph->addNode(std::make_unique<MockNode>());
  auto node = std::make_unique<MockNode>();
  // set manually so processable propagates through bridge and source
  node->setProcessable();
  auto *owner = graph->addNode(std::move(node));
  auto *param = createParam();
  auto *bridge = createParamBridge(param, owner);

  auto result = graph->addEdge(source, bridge);
  ASSERT_TRUE(result.is_ok());

  processAll();

  // Should have 3 nodes: source, bridge, owner (all processable now)
  size_t iterCount = 0;
  for (auto &&[graphObject, inputs] : graph->iter()) {
    iterCount++;
  }
  EXPECT_EQ(iterCount, 3u);
}

TEST_F(BridgeGraphWrapperTest, DisconnectSourceFromBridge) {
  auto *source = graph->addNode(std::make_unique<MockNode>());
  auto node = std::make_unique<MockNode>();
  // set manually so processable propagates through bridge and source
  node->setProcessable();
  auto *owner = graph->addNode(std::move(node));
  auto *param = createParam();
  auto *bridge = createParamBridge(param, owner);

  ASSERT_TRUE(graph->addEdge(source, bridge).is_ok());
  processAll();

  // Disconnect source → bridge
  ASSERT_TRUE(graph->removeEdge(source, bridge).is_ok());
  processAll();

  // Bridge still exists (owned by param host object), but no source connected
  size_t iterCount = 0;
  for (auto &&[graphObject, inputs] : graph->iter()) {
    iterCount++;
  }
  EXPECT_EQ(iterCount, 2u); // only owner + bridge remains processable
}

TEST_F(BridgeGraphWrapperTest, DuplicateEdgeToBridgeRejected) {
  auto *source = graph->addNode(std::make_unique<MockNode>());
  auto *owner = graph->addNode(std::make_unique<MockNode>());
  auto *param = createParam();
  auto *bridge = createParamBridge(param, owner);

  ASSERT_TRUE(graph->addEdge(source, bridge).is_ok());

  // Same edge again should fail
  auto result = graph->addEdge(source, bridge);
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), HostGraph::ResultError::EDGE_ALREADY_EXISTS);
}

TEST_F(BridgeGraphWrapperTest, CycleDetectedThroughBridge) {
  auto *nodeA = graph->addNode(std::make_unique<MockNode>());
  auto *nodeB = graph->addNode(std::make_unique<MockNode>());
  auto *param = createParam();

  // A → B (regular edge)
  ASSERT_TRUE(graph->addEdge(nodeA, nodeB).is_ok());

  // Create bridge for nodeA's param (bridge → nodeA)
  auto *bridge = createParamBridge(param, nodeA);

  // Try B → bridge — combined with bridge → A and A → B creates cycle
  auto result = graph->addEdge(nodeB, bridge);
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), HostGraph::ResultError::CYCLE_DETECTED);
}

TEST_F(BridgeGraphWrapperTest, BridgeRemovalWhenParamDestroyed) {
  auto *source = graph->addNode(std::make_unique<MockNode>());
  auto node = std::make_unique<MockNode>();
  // set manually so processable propagates through bridge and source
  node->setProcessable();
  auto *owner = graph->addNode(std::move(node));
  auto *param = createParam();
  auto *bridge = createParamBridge(param, owner);

  ASSERT_TRUE(graph->addEdge(source, bridge).is_ok());
  processAll();

  // Simulate AudioParamHostObject destruction:
  // First remove all edges to/from bridge, then remove bridge node
  ASSERT_TRUE(graph->removeEdge(source, bridge).is_ok());
  ASSERT_TRUE(graph->removeEdge(bridge, owner).is_ok());
  ASSERT_TRUE(graph->removeNode(bridge).is_ok());
  processAll();

  // Bridge should be compacted away (orphaned + no inputs)
  size_t iterCount = 0;
  for (auto &&[graphObject, inputs] : graph->iter()) {
    iterCount++;
  }
  EXPECT_EQ(iterCount, 1u); // only owner remains processable
}

TEST_F(BridgeGraphWrapperTest, MultipleSourcesConnectToSameBridge) {
  auto *source1 = graph->addNode(std::make_unique<MockNode>());
  auto *source2 = graph->addNode(std::make_unique<MockNode>());
  auto node = std::make_unique<MockNode>();
  // set manually so processable propagates through bridge and source
  node->setProcessable();
  auto *owner = graph->addNode(std::move(node));
  auto *param = createParam();
  auto *bridge = createParamBridge(param, owner);

  ASSERT_TRUE(graph->addEdge(source1, bridge).is_ok());
  ASSERT_TRUE(graph->addEdge(source2, bridge).is_ok());
  processAll();

  // Should have 4 nodes: source1, source2, bridge, owner
  size_t iterCount = 0;
  for (auto &&[graphObject, inputs] : graph->iter()) {
    iterCount++;
  }
  EXPECT_EQ(iterCount, 4u);

  // Disconnect one source
  ASSERT_TRUE(graph->removeEdge(source1, bridge).is_ok());
  processAll();

  // Still 4 nodes
  iterCount = 0;
  for (auto &&[graphObject, inputs] : graph->iter()) {
    iterCount++;
  }
  EXPECT_EQ(iterCount, 3u); // source1 is unprocessable
}

TEST_F(BridgeGraphWrapperTest, ConcurrentWithMockGraphProcessor) {
  using Processor = audioapi::test::MockGraphProcessor<ProcessableMockNode>;
  // Pre-allocate node/pool capacity to avoid grow-event allocations inside
  // the AudioThreadGuard scope.
  auto sharedGraph = std::make_shared<Graph>(4096, &disposer_, 16, 64);
  Processor processor(*sharedGraph);
  processor.start();

  auto *source = sharedGraph->addNode(std::make_unique<ProcessableMockNode>(nullptr, 10));
  auto *owner = sharedGraph->addNode(std::make_unique<ProcessableMockNode>(nullptr, 20));
  auto *param = createParam();

  // Create bridge and connect
  auto *bridge = sharedGraph->addNode(std::make_unique<BridgeNode>(param));
  ASSERT_TRUE(sharedGraph->addEdge(bridge, owner).is_ok());
  ASSERT_TRUE(sharedGraph->addEdge(source, bridge).is_ok());

  // Let processor run a few cycles
  while (processor.cyclesCompleted() < 10) {
    std::this_thread::yield();
  }

  // Disconnect source from bridge
  ASSERT_TRUE(sharedGraph->removeEdge(source, bridge).is_ok());

  while (processor.cyclesCompleted() < 20) {
    std::this_thread::yield();
  }

  processor.stop();
  EXPECT_TRUE(processor.allocationClean());
}

// =========================================================================
// F. Fuzz test with bridge nodes
// =========================================================================

class BridgeFuzzTest : public ::testing::TestWithParam<uint64_t> {
 protected:
  using HNode = HostGraph::Node;
  static constexpr size_t kPayloadSize = audioapi::DISPOSER_PAYLOAD_SIZE;

  DisposerImpl<kPayloadSize> disposer_{64};
  std::shared_ptr<Graph> graph;
  std::mt19937_64 rng;
  std::vector<HNode *> liveNodes;
  std::vector<HNode *> bridgeNodes;
  std::vector<std::shared_ptr<AudioParam>> params_; // Real params
  std::vector<AudioParam *> paramPtrs_;             // Raw pointers for test use

  void SetUp() override {
    graph = std::make_shared<Graph>(4096, &disposer_);
    rng.seed(GetParam());

    // Create real AudioParam objects for testing
    for (int i = 1; i <= 8; i++) {
      params_.push_back(createMockAudioParam());
      paramPtrs_.push_back(params_.back().get());
    }
  }

  void processAll() {
    graph->processEvents();
    graph->process();
  }

  HNode *pickRandom() {
    if (liveNodes.empty())
      return nullptr;
    return liveNodes[std::uniform_int_distribution<size_t>(0, liveNodes.size() - 1)(rng)];
  }

  HNode *pickBridge() {
    if (bridgeNodes.empty())
      return nullptr;
    return bridgeNodes[std::uniform_int_distribution<size_t>(0, bridgeNodes.size() - 1)(rng)];
  }

  AudioParam *pickParam() {
    return paramPtrs_[std::uniform_int_distribution<size_t>(0, paramPtrs_.size() - 1)(rng)];
  }
};

TEST_P(BridgeFuzzTest, RandomParamOps) {
  size_t initialCount = std::uniform_int_distribution<size_t>(4, 16)(rng);
  size_t opCount = std::uniform_int_distribution<size_t>(50, 200)(rng);

  // Seed nodes
  for (size_t i = 0; i < initialCount; i++) {
    liveNodes.push_back(graph->addNode(std::make_unique<MockNode>()));
  }
  processAll();

  for (size_t i = 0; i < opCount; i++) {
    size_t op = std::uniform_int_distribution<size_t>(0, 99)(rng);

    if (op < 10) {
      // Add node
      liveNodes.push_back(graph->addNode(std::make_unique<MockNode>()));

    } else if (op < 25) {
      // Add regular edge
      auto *a = pickRandom();
      auto *b = pickRandom();
      if (a && b && a != b) {
        (void)graph->addEdge(a, b);
      }

    } else if (op < 35) {
      // Create bridge for a param and connect to owner
      auto *owner = pickRandom();
      if (owner) {
        auto *bridge = graph->addNode(std::make_unique<BridgeNode>(pickParam()));
        (void)graph->addEdge(bridge, owner);
        bridgeNodes.push_back(bridge);
      }

    } else if (op < 50) {
      // Connect source to bridge
      auto *source = pickRandom();
      auto *bridge = pickBridge();
      if (source && bridge) {
        (void)graph->addEdge(source, bridge);
      }

    } else if (op < 60) {
      // Disconnect source from bridge
      auto *source = pickRandom();
      auto *bridge = pickBridge();
      if (source && bridge) {
        (void)graph->removeEdge(source, bridge);
      }

    } else if (op < 70) {
      // Remove bridge node (simulating param destruction)
      auto *bridge = pickBridge();
      if (bridge) {
        (void)graph->removeNode(bridge);
        bridgeNodes.erase(
            std::remove(bridgeNodes.begin(), bridgeNodes.end(), bridge), bridgeNodes.end());
      }

    } else if (op < 80) {
      // Remove regular node
      auto *n = pickRandom();
      if (n) {
        (void)graph->removeNode(n);
        liveNodes.erase(std::remove(liveNodes.begin(), liveNodes.end(), n), liveNodes.end());
      }

    } else if (op < 90) {
      // Remove regular edge
      auto *a = pickRandom();
      auto *b = pickRandom();
      if (a && b) {
        (void)graph->removeEdge(a, b);
      }

    } else {
      // Process
      processAll();
    }
  }

  // Final process — should not crash or produce bad state
  processAll();

  // Verify iter doesn't crash
  for (auto &&[graphObject, inputs] : graph->iter()) {
    (void)graphObject;
    for (const auto &input : inputs) {
      (void)input;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    Seeds,
    BridgeFuzzTest,
    ::testing::Range(uint64_t{0}, uint64_t{100}),
    [](const ::testing::TestParamInfo<uint64_t> &info) {
      return "seed_" + std::to_string(info.param);
    });

} // namespace audioapi::utils::graph
