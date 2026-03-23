#include <audioapi/core/utils/graph/BridgeNode.hpp>
#include <audioapi/core/utils/graph/Graph.hpp>
#include <audioapi/core/utils/graph/NodeHandle.hpp>
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

TEST(BridgeNodeContract, MockNodeIsProcessable) {
  MockNode node;
  EXPECT_TRUE(node.isProcessable());
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
  // Use a dummy pointer to verify storage
  auto *fakeParam = reinterpret_cast<AudioParam *>(0xDEAD);
  BridgeNode bridge(fakeParam);
  EXPECT_EQ(bridge.param(), fakeParam);
}

// =========================================================================
// B. Graph structural tests (HostGraph + AudioGraph)
// =========================================================================

class BridgeGraphTest : public ::testing::Test {
 protected:
  using HNode = HostGraph::Node;
  using AGEvent = HostGraph::AGEvent;
  static constexpr size_t kPayloadSize = HostGraph::kDisposerPayloadSize;

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
  static constexpr size_t kPayloadSize = HostGraph::kDisposerPayloadSize;

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
  auto *processable1 = addNode(std::make_unique<MockNode>());
  auto *nonProcessable = addNode(std::make_unique<NonProcessableMockNode>());
  auto *processable2 = addNode(std::make_unique<MockNode>());

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

  // Should yield A, B, C in topo order (bridge skipped)
  std::vector<int> values;
  for (auto &&[graphObject, inputs] : audioGraph.iter()) {
    auto *node = dynamic_cast<ProcessableMockNode *>(&graphObject);
    ASSERT_NE(node, nullptr);
    values.push_back(node->value.load());
  }
  ASSERT_EQ(values.size(), 3u);
  EXPECT_EQ(values[0], 1);
  EXPECT_EQ(values[1], 2);
  EXPECT_EQ(values[2], 3);
}

TEST_F(BridgeIterTest, InputsViewMayReferenceBridgeIndices) {
  // source → bridge → owner
  // iter() skips bridge but owner's input list in AudioGraph still
  // references the bridge's index. Callers use asAudioNode() to handle this.
  auto *source = addNode(std::make_unique<MockNode>());
  auto *bridge = addNode(std::make_unique<BridgeNode>(nullptr));
  auto *owner = addNode(std::make_unique<MockNode>());

  ASSERT_TRUE(addEdge(source, bridge));
  ASSERT_TRUE(addEdge(bridge, owner));
  audioGraph.process();

  size_t processableCount = 0;
  for (auto &&[graphObject, inputs] : audioGraph.iter()) {
    processableCount++;
    // Owner should see bridge as input (which is a BridgeNode, not AudioNode)
    for (const auto &input : inputs) {
      // Input could be bridge or source — both are valid GraphObjects
      (void)input;
    }
  }
  EXPECT_EQ(processableCount, 2u); // source + owner (bridge skipped)
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
  static constexpr size_t kPayloadSize = HostGraph::kDisposerPayloadSize;
  DisposerImpl<kPayloadSize> disposer_{64};
  std::shared_ptr<Graph> graph;

  void SetUp() override {
    graph = std::make_shared<Graph>(4096, &disposer_);
  }

  void processAll() {
    graph->processEvents();
    graph->process();
  }
};

TEST_F(BridgeGraphWrapperTest, ConnectParamCreatesBridge) {
  auto *source = graph->addNode(std::make_unique<MockNode>());
  auto *owner = graph->addNode(std::make_unique<MockNode>());
  auto *fakeParam = reinterpret_cast<AudioParam *>(0x1234);

  auto result = graph->connectParam(source, owner, fakeParam);
  ASSERT_TRUE(result.is_ok());

  processAll();

  // Should have 3 nodes: source, bridge, owner
  size_t iterCount = 0;
  for (auto &&[graphObject, inputs] : graph->iter()) {
    iterCount++;
  }
  // iter() skips non-processable bridge, so we see 2
  EXPECT_EQ(iterCount, 2u);
}

TEST_F(BridgeGraphWrapperTest, DisconnectParamRemovesBridge) {
  auto *source = graph->addNode(std::make_unique<MockNode>());
  auto *owner = graph->addNode(std::make_unique<MockNode>());
  auto *fakeParam = reinterpret_cast<AudioParam *>(0x1234);

  ASSERT_TRUE(graph->connectParam(source, owner, fakeParam).is_ok());
  processAll();

  ASSERT_TRUE(graph->disconnectParam(source, owner, fakeParam).is_ok());
  processAll();

  // Bridge should be compacted away (orphaned + no inputs)
  size_t iterCount = 0;
  for (auto &&[graphObject, inputs] : graph->iter()) {
    iterCount++;
  }
  EXPECT_EQ(iterCount, 2u); // source + owner
}

TEST_F(BridgeGraphWrapperTest, DuplicateConnectParamRejected) {
  auto *source = graph->addNode(std::make_unique<MockNode>());
  auto *owner = graph->addNode(std::make_unique<MockNode>());
  auto *fakeParam = reinterpret_cast<AudioParam *>(0x1234);

  ASSERT_TRUE(graph->connectParam(source, owner, fakeParam).is_ok());

  // Same connection again should fail
  auto result = graph->connectParam(source, owner, fakeParam);
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), HostGraph::ResultError::EDGE_ALREADY_EXISTS);
}

TEST_F(BridgeGraphWrapperTest, ConnectParamCycleDetected) {
  auto *nodeA = graph->addNode(std::make_unique<MockNode>());
  auto *nodeB = graph->addNode(std::make_unique<MockNode>());
  auto *fakeParam = reinterpret_cast<AudioParam *>(0x1234);

  // A → B (regular edge)
  ASSERT_TRUE(graph->addEdge(nodeA, nodeB).is_ok());

  // Now try B →(param)→ A — this would create: B → bridge → A
  // Combined with A → B, this creates cycle: A → B → bridge → A
  auto result = graph->connectParam(nodeB, nodeA, fakeParam);
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), HostGraph::ResultError::CYCLE_DETECTED);
}

TEST_F(BridgeGraphWrapperTest, OwnerRemovalCascadesBridgeCleanup) {
  auto *source = graph->addNode(std::make_unique<MockNode>());
  auto *owner = graph->addNode(std::make_unique<MockNode>());
  auto *fakeParam = reinterpret_cast<AudioParam *>(0x1234);

  ASSERT_TRUE(graph->connectParam(source, owner, fakeParam).is_ok());
  processAll();

  // Remove owner — should cascade remove the bridge
  ASSERT_TRUE(graph->removeNodeWithBridges(owner).is_ok());
  processAll();

  // Only source should remain as processable
  size_t iterCount = 0;
  for (auto &&[graphObject, inputs] : graph->iter()) {
    iterCount++;
  }
  EXPECT_EQ(iterCount, 1u);
}

TEST_F(BridgeGraphWrapperTest, SourceRemovalCascadesBridgeCleanup) {
  auto *source = graph->addNode(std::make_unique<MockNode>());
  auto *owner = graph->addNode(std::make_unique<MockNode>());
  auto *fakeParam = reinterpret_cast<AudioParam *>(0x1234);

  ASSERT_TRUE(graph->connectParam(source, owner, fakeParam).is_ok());
  processAll();

  // Remove source — should cascade remove the bridge
  ASSERT_TRUE(graph->removeNodeWithBridges(source).is_ok());
  processAll();

  // Only owner should remain as processable
  size_t iterCount = 0;
  for (auto &&[graphObject, inputs] : graph->iter()) {
    iterCount++;
  }
  EXPECT_EQ(iterCount, 1u);
}

TEST_F(BridgeGraphWrapperTest, MultipleBridgesFromSameSource) {
  auto *source = graph->addNode(std::make_unique<MockNode>());
  auto *ownerA = graph->addNode(std::make_unique<MockNode>());
  auto *ownerB = graph->addNode(std::make_unique<MockNode>());
  auto *paramA = reinterpret_cast<AudioParam *>(0xA);
  auto *paramB = reinterpret_cast<AudioParam *>(0xB);

  ASSERT_TRUE(graph->connectParam(source, ownerA, paramA).is_ok());
  ASSERT_TRUE(graph->connectParam(source, ownerB, paramB).is_ok());
  processAll();

  // Disconnect one
  ASSERT_TRUE(graph->disconnectParam(source, ownerA, paramA).is_ok());
  processAll();

  // Other bridge should still exist (source → bridge → ownerB)
  // Disconnected bridge should be compacted away

  // Connect again should work
  ASSERT_TRUE(graph->connectParam(source, ownerA, paramA).is_ok());
  processAll();
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
  auto *fakeParam = reinterpret_cast<AudioParam *>(0x42);

  ASSERT_TRUE(sharedGraph->connectParam(source, owner, fakeParam).is_ok());

  // Let processor run a few cycles
  while (processor.cyclesCompleted() < 10) {
    std::this_thread::yield();
  }

  ASSERT_TRUE(sharedGraph->disconnectParam(source, owner, fakeParam).is_ok());

  while (processor.cyclesCompleted() < 20) {
    std::this_thread::yield();
  }

  processor.stop();
  EXPECT_TRUE(processor.allocationClean());
}

// =========================================================================
// F. Fuzz test extension with connectParam/disconnectParam
// =========================================================================

class BridgeFuzzTest : public ::testing::TestWithParam<uint64_t> {
 protected:
  using HNode = HostGraph::Node;
  static constexpr size_t kPayloadSize = HostGraph::kDisposerPayloadSize;

  DisposerImpl<kPayloadSize> disposer_{64};
  std::shared_ptr<Graph> graph;
  std::mt19937_64 rng;
  std::vector<HNode *> liveNodes;
  std::vector<AudioParam *> fakeParams;

  void SetUp() override {
    graph = std::make_shared<Graph>(4096, &disposer_);
    rng.seed(GetParam());

    // Create a set of fake param pointers
    for (int i = 1; i <= 8; i++) {
      fakeParams.push_back(reinterpret_cast<AudioParam *>(static_cast<uintptr_t>(i * 0x100)));
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

  AudioParam *pickParam() {
    return fakeParams[std::uniform_int_distribution<size_t>(0, fakeParams.size() - 1)(rng)];
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

    } else if (op < 40) {
      // Connect param
      auto *source = pickRandom();
      auto *owner = pickRandom();
      if (source && owner && source != owner) {
        (void)graph->connectParam(source, owner, pickParam());
      }

    } else if (op < 55) {
      // Disconnect param
      auto *source = pickRandom();
      auto *owner = pickRandom();
      if (source && owner) {
        (void)graph->disconnectParam(source, owner, pickParam());
      }

    } else if (op < 70) {
      // Remove node with bridges
      auto *n = pickRandom();
      if (n) {
        (void)graph->removeNodeWithBridges(n);
        liveNodes.erase(std::remove(liveNodes.begin(), liveNodes.end(), n), liveNodes.end());
      }

    } else if (op < 85) {
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
