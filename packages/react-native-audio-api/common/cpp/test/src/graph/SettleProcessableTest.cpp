#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/AudioGraph.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <gtest/gtest.h>

#include <memory>
#include <utility>

#include "TestGraphUtils.h"

namespace audioapi::utils::graph {

// A node that always reports processable regardless of processableState_,
// mimicking a tail-bearing node still draining after a downstream disconnect.
// settleProcessableState() must key off processableState_ (which stays
// NOT_PROCESSABLE here), never isProcessable(), so it must NOT re-activate this
// node's upstream inputs.
struct TailMockNode : MockNode {
  [[nodiscard]] bool isProcessable() const override {
    return true;
  }
};

// Exposes the protected disable() hook so tests can drive the sticky-idle path.
struct DisableMockNode : MockNode {
  void doDisable() {
    disable();
  }
};

// ── Low-level fixture: drive AudioGraph + HostGraph directly + settle ───────
class SettleProcessableTest : public ::testing::Test {
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

  bool removeEdge(HNode *from, HNode *to) {
    auto result = hostGraph.removeEdge(from, to);
    if (result.is_ok()) {
      std::move(result).unwrap()(audioGraph, disposer_);
      return true;
    }
    return false;
  }

  void settleOnly() {
    audioGraph.process();
    audioGraph.settleProcessableState();
  }

  void runQuantum() {
    settleOnly();
    for (auto &&[node, inputs] : audioGraph.iter()) {
      node.process(inputs, audioapi::RENDER_QUANTUM_SIZE);
    }
  }

  static bool processable(HNode *node) {
    return node->handle->audioNode->isProcessable();
  }
};

// Connecting a chain into an ALWAYS seed pulls the whole upstream chain
// processable — no HostGraph processable walk involved.
TEST_F(SettleProcessableTest, ChainIntoSeedBecomesProcessable) {
  auto *source = addNode(std::make_unique<MockNode>());
  auto *mid = addNode(std::make_unique<MockNode>());
  auto seedNode = std::make_unique<MockNode>();
  seedNode->setProcessable(); // ALWAYS seed
  auto *seed = addNode(std::move(seedNode));

  ASSERT_TRUE(addEdge(source, mid));
  ASSERT_TRUE(addEdge(mid, seed));

  settleOnly();

  EXPECT_TRUE(processable(source));
  EXPECT_TRUE(processable(mid));
  EXPECT_TRUE(processable(seed));
}

// Disconnecting the last processable consumer deactivates conditional upstream.
TEST_F(SettleProcessableTest, DisconnectLastConsumerDeactivatesChain) {
  auto *source = addNode(std::make_unique<MockNode>());
  auto *mid = addNode(std::make_unique<MockNode>());
  auto seedNode = std::make_unique<MockNode>();
  seedNode->setProcessable();
  auto *seed = addNode(std::move(seedNode));

  ASSERT_TRUE(addEdge(source, mid));
  ASSERT_TRUE(addEdge(mid, seed));
  settleOnly();
  ASSERT_TRUE(processable(source));

  // Disconnect mid -> seed: nothing processable pulls the chain any more.
  ASSERT_TRUE(removeEdge(mid, seed));
  runQuantum();

  EXPECT_FALSE(processable(source));
  EXPECT_FALSE(processable(mid));
  EXPECT_TRUE(processable(seed)); // ALWAYS seed stays on
}

// A draining tail node stays in iter() (isProcessable() override) but its
// NOT_PROCESSABLE state must NOT re-activate its upstream cone.
TEST_F(SettleProcessableTest, TailNodeDoesNotReactivateUpstream) {
  auto *upstream = addNode(std::make_unique<MockNode>());
  auto *tail = addNode(std::make_unique<TailMockNode>());

  ASSERT_TRUE(addEdge(upstream, tail));
  runQuantum();

  // Tail is scheduled (drains) but its input is not pulled processable.
  EXPECT_TRUE(processable(tail));
  EXPECT_FALSE(processable(upstream));

  size_t count = 0;
  for (auto &&[graphObject, inputs] : audioGraph.iter()) {
    (void)inputs;
    (void)graphObject;
    count++;
  }
  EXPECT_EQ(count, 1u); // only the tail node
}

// ── Wrapper fixture: exercise Graph::linkNodes + settle via process() ───────
class SettleLinkTest : public ::testing::Test {
 protected:
  using HNode = HostGraph::Node;
  static constexpr size_t kPayloadSize = audioapi::DISPOSER_PAYLOAD_SIZE;
  DisposerImpl<kPayloadSize> disposer_{64};
  std::shared_ptr<Graph> graph;

  void SetUp() override {
    graph = std::make_shared<Graph>(4096, &disposer_);
  }

  void processAll() {
    graph->processEvents();
    graph->process();
    for (auto &&[node, inputs] : graph->iter()) {
      node.process(inputs, audioapi::RENDER_QUANTUM_SIZE);
    }
  }

  size_t iterCount() {
    size_t count = 0;
    for (auto &&[graphObject, inputs] : graph->iter()) {
      (void)graphObject;
      (void)inputs;
      count++;
    }
    return count;
  }
};

// A processable-link (DelayReader -> DelayWriter analogue) pulls the link
// target and, transitively, the target's own audio inputs — even though there
// is no audio edge between reader and writer.
TEST_F(SettleLinkTest, LinkPullsTargetAndItsInputs) {
  auto seedNode = std::make_unique<MockNode>();
  seedNode->setProcessable(); // ALWAYS seed (destination analogue)
  auto *consumer = graph->addNode(std::move(seedNode));

  auto *reader = graph->addNode(std::make_unique<MockNode>());
  auto *writer = graph->addNode(std::make_unique<MockNode>());
  auto *source = graph->addNode(std::make_unique<MockNode>());

  // Audio edges: source -> writer, reader -> consumer.
  ASSERT_TRUE(graph->addEdge(source, writer).is_ok());
  ASSERT_TRUE(graph->addEdge(reader, consumer).is_ok());
  // Processable link reader -> writer (no audio edge).
  graph->linkNodes(reader, writer);

  processAll();

  // After a full quantum only the ALWAYS consumer remains in iter().
  EXPECT_EQ(iterCount(), 1u);
}

// Disconnecting the reader from the seed tears the linked writer (and its
// inputs) back down.
TEST_F(SettleLinkTest, LinkDeactivatesOnDisconnect) {
  auto seedNode = std::make_unique<MockNode>();
  seedNode->setProcessable();
  auto *consumer = graph->addNode(std::move(seedNode));

  auto *reader = graph->addNode(std::make_unique<MockNode>());
  auto *writer = graph->addNode(std::make_unique<MockNode>());
  auto *source = graph->addNode(std::make_unique<MockNode>());

  ASSERT_TRUE(graph->addEdge(source, writer).is_ok());
  ASSERT_TRUE(graph->addEdge(reader, consumer).is_ok());
  graph->linkNodes(reader, writer);
  processAll();
  ASSERT_EQ(iterCount(), 1u);

  ASSERT_TRUE(graph->removeEdge(reader, consumer).is_ok());
  processAll();

  // Only the ALWAYS seed remains processable after the quantum completes.
  EXPECT_EQ(iterCount(), 1u);
}

// disable() is sticky: a finished source connected to a processable consumer
// must not be re-activated by the every-quantum reverse pull.
TEST_F(SettleLinkTest, DisabledSourceIsNotReactivated) {
  auto seedNode = std::make_unique<MockNode>();
  seedNode->setProcessable();
  auto *consumer = graph->addNode(std::move(seedNode));

  auto srcNode = std::make_unique<DisableMockNode>();
  auto *srcRaw = srcNode.get();
  auto *source = graph->addNode(std::move(srcNode));

  ASSERT_TRUE(graph->addEdge(source, consumer).is_ok());
  processAll();
  EXPECT_EQ(iterCount(), 1u); // ALWAYS consumer only after demotion

  // Source finishes playback: disable() flips it off and opts out of the pull.
  srcRaw->doDisable();
  processAll();

  EXPECT_EQ(iterCount(), 1u); // consumer only; source stays idle
}

// Idle sources keep a non-null getOutput() with stale samples. settle zeros
// those buffers before the render loop so consumers hear silence (fixes
// audionode-channel-rules WPT ghost echoes).
TEST_F(SettleLinkTest, IdleInputBuffersAreZeroed) {
  auto consumerNode = std::make_unique<MockNode>();
  consumerNode->setProcessable();
  auto *consumer = graph->addNode(std::move(consumerNode));

  auto srcNode = std::make_unique<DisableMockNode>();
  auto *srcRaw = srcNode.get();
  auto *source = graph->addNode(std::move(srcNode));

  ASSERT_TRUE(graph->addEdge(source, consumer).is_ok());
  processAll();

  (*srcRaw->getOutputBuffer()->getChannel(0))[0] = 1.0f;
  srcRaw->doDisable();
  processAll();

  EXPECT_FALSE(srcRaw->isProcessable());
  EXPECT_NE(srcRaw->getOutput(), nullptr);
  EXPECT_FLOAT_EQ((*srcRaw->getOutputBuffer()->getChannel(0))[0], 0.0f);
}

} // namespace audioapi::utils::graph
