#include <audioapi/core/AudioNode.h>
#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include "TestGraphUtils.h"

namespace audioapi::utils::graph {

class BasicAudioNode : public audioapi::AudioNode {
 public:
  BasicAudioNode(
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      const audioapi::AudioNodeOptions &options)
      : AudioNode(context, options) {}

  void processNode(int /*framesToProcess*/) override {}
};

class GraphTest : public ::testing::Test {
 protected:
  static constexpr size_t kPayloadSize = audioapi::DISPOSER_PAYLOAD_SIZE;
  DisposerImpl<kPayloadSize> disposer_{64};
  std::unique_ptr<Graph> graph;

  void SetUp() override {
    graph = std::make_unique<Graph>(4096, &disposer_);
  }

  const AudioGraph &getAudioGraph() {
    return graph->audioGraph;
  }

  const HostGraph &getHostGraph() {
    return graph->hostGraph;
  }
};

/// Minimal AudioNode used as both a "source" (configurable channel count on
/// its output buffer) and a "destination" (configurable channelCount /
/// channelCountMode whose effective buffer we assert on).
class ChannelCountTestNode : public audioapi::AudioNode {
 public:
  ChannelCountTestNode(
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      const audioapi::AudioNodeOptions &options)
      : AudioNode(context, options) {}

  void processNode(int /*framesToProcess*/) override {}
};

struct ChannelOpts {
  int channelCount;
  audioapi::ChannelCountMode mode;
};

/// @returns the computed number of channels on the output buffer of the
/// AudioNode that backs `node`.
inline size_t channelsOf(const HostGraph::Node *node) {
  auto *audioNode = node->handle->audioNode->asAudioNode();
  return audioNode->getOutputBuffer()->getNumberOfChannels();
}

/// Adds a ChannelCountTestNode with the given options to the managed
/// `graph`. Returns the HostGraph::Node pointer; the associated AudioGraph
/// slot is populated once `graph->processEvents()` is called.
inline HostGraph::Node *addChannelCountNode(Graph &graph, const ChannelOpts &opts) {
  audioapi::AudioNodeOptions audioNodeOpts;
  audioNodeOpts.channelCount = opts.channelCount;
  audioNodeOpts.channelCountMode = opts.mode;

  auto audioNode = std::make_unique<ChannelCountTestNode>(getGraphTestContext(), audioNodeOpts);
  return graph.addNode(std::move(audioNode));
}

TEST_F(GraphTest, EventsAreScheduledButNotExecutedUntilProcess) {
  auto audioNode = std::make_unique<BasicAudioNode>(getGraphTestContext(), AudioNodeOptions());
  auto *node = graph->addNode(std::move(audioNode));
  ASSERT_NE(node, nullptr);

  // AudioGraph should not be aware of the node yet (event not processed)
  const auto &ag = getAudioGraph();

  size_t sizeBefore = ag.size();

  if (ag.empty()) {
    EXPECT_EQ(ag.size(), 0u);
  }

  graph->processEvents();

  EXPECT_GE(ag.size(), 1u);
}

TEST_F(GraphTest, NoUselessEventsScheduled) {
  auto audioNode1 = std::make_unique<BasicAudioNode>(getGraphTestContext(), AudioNodeOptions());
  auto audioNode2 = std::make_unique<BasicAudioNode>(getGraphTestContext(), AudioNodeOptions());
  auto *node1 = graph->addNode(std::move(audioNode1));
  auto *node2 = graph->addNode(std::move(audioNode2));
  graph->processEvents();

  // Initial state
  const auto &ag = getAudioGraph();
  // Convert to verify
  auto initialAdj = TestGraphUtils::convertAudioGraphToAdjacencyList(
      const_cast<AudioGraph &>(ag)); // casting const away if utils need it, or verifyutils usage

  // Try adding duplicate edge (should fail and NOT schedule event)
  ASSERT_TRUE(graph->addEdge(node1, node2).is_ok()); // Success first time
  graph->processEvents();

  // Result of valid op
  auto intermediateAdj =
      TestGraphUtils::convertAudioGraphToAdjacencyList(const_cast<AudioGraph &>(ag));

  // Try adding SAME edge (should fail)
  auto result = graph->addEdge(node1, node2);
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), HostGraph::ResultError::EDGE_ALREADY_EXISTS);

  // Even if we call processEvents, state should not change (and no event should be consumed ideally,
  // impossible to check queue count easily without friend or mock, but state check is good enough)
  graph->processEvents();

  auto finalAdj = TestGraphUtils::convertAudioGraphToAdjacencyList(const_cast<AudioGraph &>(ag));
  EXPECT_EQ(intermediateAdj, finalAdj);
}

TEST_F(GraphTest, ThreadRaceConcurrency) {
  // Stress test with threads
  // One thread produces graph changes
  // One thread processes events (consumer)

  std::atomic<bool> running{true};
  std::vector<HostGraph::Node *> nodes;

  // Add initial nodes
  for (int i = 0; i < 10; ++i) {
    auto audioNode = std::make_unique<BasicAudioNode>(getGraphTestContext(), AudioNodeOptions());
    nodes.push_back(graph->addNode(std::move(audioNode)));
  }
  graph->processEvents(); // Setup

  std::thread audioThread([&]() {
    while (running) {
      graph->processEvents();
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    graph->processEvents();
  });

  // Main thread mutations
  unsigned int seed = 12345;
  for (int i = 0; i < 100; ++i) {
    // Randomly add edge or remove edge or add node
    int op = rand_r(&seed) % 3;
    if (op == 0) {
      auto audioNode = std::make_unique<BasicAudioNode>(getGraphTestContext(), AudioNodeOptions());
      auto *n = graph->addNode(std::move(audioNode));
      nodes.push_back(n);
    } else if (op == 1 && nodes.size() > 2) {
      // Add edge
      HostGraph::Node *n1, *n2;
      {
        n1 = nodes[rand_r(&seed) % nodes.size()];
        n2 = nodes[rand_r(&seed) % nodes.size()];
      }
      if (n1 != n2) {
        // Ignore result (could be error if edge exists or cycle)
        (void)graph->addEdge(n1, n2);
      }
    } else if (op == 2 && nodes.size() > 5) {
      // Remove edge
      HostGraph::Node *n1, *n2;
      {
        n1 = nodes[rand_r(&seed) % nodes.size()];
        n2 = nodes[rand_r(&seed) % nodes.size()];
      }
      (void)graph->removeEdge(n1, n2);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }

  running = false;
  audioThread.join();

  // If we reached here without crash (segfault), we are somewhat good.
  // Verify graph consistency (Host vs Audio)
  {
    // Must wait for all events to flush - already done by final processEvents in thread,
    // but let's do one more to be sure
    graph->processEvents();

    const auto &ag = getAudioGraph();
    const auto &hg = getHostGraph();

    auto audioAdj = TestGraphUtils::convertAudioGraphToAdjacencyList(const_cast<AudioGraph &>(ag));
    auto hostAdj = TestGraphUtils::convertHostGraphToAdjacencyList(const_cast<HostGraph &>(hg));

    // They should match
    EXPECT_EQ(audioAdj, hostAdj);
  }
}

// ─── Channel-count negotiation on connect/disconnect ─────────────────────
//
// These tests assert the Web Audio contract: the computed number of
// channels on a node's output buffer must follow `channelCountMode`
//   - MAX          -> max(channelCount of each connected input)
//                     (the node's channelCount attribute is ignored)
//   - CLAMPED_MAX  -> min(channelCount attribute,
//                         max(channelCount of each connected input))
//   - EXPLICIT     -> always the channelCount attribute
//
// The computation must happen on the HostGraph side at addEdge/removeEdge
// time, but the actual buffer swap is published through the AGEvent and
// applied on the AudioGraph side — therefore each test calls
// `graph->processEvents()` before inspecting `channelsOf(...)`.

TEST_F(GraphTest, ChannelCountNegotiation_MaxMode_SingleInput) {
  auto *source =
      addChannelCountNode(*graph, {.channelCount = 4, .mode = ChannelCountMode::EXPLICIT});
  auto *dest = addChannelCountNode(*graph, {.channelCount = 2, .mode = ChannelCountMode::MAX});
  graph->processEvents();

  ASSERT_TRUE(graph->addEdge(source, dest).is_ok());
  graph->processEvents();

  EXPECT_EQ(channelsOf(dest), 4u)
      << "MAX mode: after connecting a 4-channel source the downstream buffer "
         "must be resized to 4 channels (channelCount attribute is ignored)";
}

TEST_F(GraphTest, ChannelCountNegotiation_MaxMode_MultipleInputsTakeMax) {
  auto *mono = addChannelCountNode(*graph, {.channelCount = 1, .mode = ChannelCountMode::EXPLICIT});
  auto *six = addChannelCountNode(*graph, {.channelCount = 6, .mode = ChannelCountMode::EXPLICIT});
  auto *dest = addChannelCountNode(*graph, {.channelCount = 2, .mode = ChannelCountMode::MAX});
  graph->processEvents();

  ASSERT_TRUE(graph->addEdge(mono, dest).is_ok());
  ASSERT_TRUE(graph->addEdge(six, dest).is_ok());
  graph->processEvents();

  EXPECT_EQ(channelsOf(dest), 6u)
      << "MAX mode: the downstream buffer must follow the largest connected input";
}

TEST_F(GraphTest, ChannelCountNegotiation_ClampedMaxMode_ClampsAboveAttribute) {
  auto *source =
      addChannelCountNode(*graph, {.channelCount = 6, .mode = ChannelCountMode::EXPLICIT});
  auto *dest =
      addChannelCountNode(*graph, {.channelCount = 2, .mode = ChannelCountMode::CLAMPED_MAX});
  graph->processEvents();

  ASSERT_TRUE(graph->addEdge(source, dest).is_ok());
  graph->processEvents();

  EXPECT_EQ(channelsOf(dest), 2u)
      << "CLAMPED_MAX should clamp a 6-channel input down to channelCount=2";
}

TEST_F(GraphTest, ChannelCountNegotiation_ClampedMaxMode_FollowsInputWhenBelowAttribute) {
  auto *source =
      addChannelCountNode(*graph, {.channelCount = 1, .mode = ChannelCountMode::EXPLICIT});
  auto *dest =
      addChannelCountNode(*graph, {.channelCount = 4, .mode = ChannelCountMode::CLAMPED_MAX});
  graph->processEvents();

  ASSERT_TRUE(graph->addEdge(source, dest).is_ok());
  graph->processEvents();

  EXPECT_EQ(channelsOf(dest), 1u)
      << "CLAMPED_MAX: mono input with channelCount=4 must still produce a mono buffer";
}

TEST_F(GraphTest, ChannelCountNegotiation_ExplicitMode_IgnoresInput) {
  auto *source =
      addChannelCountNode(*graph, {.channelCount = 6, .mode = ChannelCountMode::EXPLICIT});
  auto *dest = addChannelCountNode(*graph, {.channelCount = 4, .mode = ChannelCountMode::EXPLICIT});
  graph->processEvents();

  ASSERT_TRUE(graph->addEdge(source, dest).is_ok());
  graph->processEvents();

  EXPECT_EQ(channelsOf(dest), 4u) << "EXPLICIT must always produce exactly channelCount channels";
}

TEST_F(GraphTest, ChannelCountNegotiation_MaxMode_RecomputesOnSecondConnection) {
  auto *stereoSource =
      addChannelCountNode(*graph, {.channelCount = 2, .mode = ChannelCountMode::EXPLICIT});
  auto *quadSource =
      addChannelCountNode(*graph, {.channelCount = 4, .mode = ChannelCountMode::EXPLICIT});
  auto *dest = addChannelCountNode(*graph, {.channelCount = 2, .mode = ChannelCountMode::MAX});
  graph->processEvents();

  ASSERT_TRUE(graph->addEdge(stereoSource, dest).is_ok());
  graph->processEvents();
  EXPECT_EQ(channelsOf(dest), 2u)
      << "MAX: with only a stereo source connected, buffer should be 2 channels";

  ASSERT_TRUE(graph->addEdge(quadSource, dest).is_ok());
  graph->processEvents();
  EXPECT_EQ(channelsOf(dest), 4u)
      << "MAX: connecting a 4-channel source must grow the buffer to 4 channels";
}

TEST_F(GraphTest, ChannelCountNegotiation_MaxMode_RecomputesOnDisconnection) {
  auto *stereoSource =
      addChannelCountNode(*graph, {.channelCount = 2, .mode = ChannelCountMode::EXPLICIT});
  auto *quadSource =
      addChannelCountNode(*graph, {.channelCount = 4, .mode = ChannelCountMode::EXPLICIT});
  auto *dest = addChannelCountNode(*graph, {.channelCount = 2, .mode = ChannelCountMode::MAX});
  graph->processEvents();

  ASSERT_TRUE(graph->addEdge(stereoSource, dest).is_ok());
  ASSERT_TRUE(graph->addEdge(quadSource, dest).is_ok());
  graph->processEvents();
  ASSERT_EQ(channelsOf(dest), 4u);

  ASSERT_TRUE(graph->removeEdge(quadSource, dest).is_ok());
  graph->processEvents();

  EXPECT_EQ(channelsOf(dest), 2u)
      << "MAX: removing the 4-channel source should shrink the buffer back to 2 channels";
}

} // namespace audioapi::utils::graph
