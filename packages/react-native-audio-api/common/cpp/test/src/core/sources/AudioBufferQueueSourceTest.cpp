#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr int SAMPLE_RATE = 24000;
constexpr int QUANTUM = RENDER_QUANTUM_SIZE;

/// Builds a mono AudioBuffer of `frames` frames whose samples continue a
/// global ramp: sample k holds the value (startValue + k) * 1e-6f. Feeding
/// consecutive ramp buffers lets assertions detect skipped frames, repeated
/// frames, and stale/garbage samples in a single pass.
std::shared_ptr<AudioBuffer> makeRampBuffer(size_t frames, size_t startValue) {
  auto buffer = std::make_shared<AudioBuffer>(frames, 1, (float)SAMPLE_RATE);
  auto channel = buffer->getChannel(0)->span();
  for (size_t i = 0; i < frames; ++i) {
    channel[i] = static_cast<float>(startValue + i) * 1e-6f;
  }
  return buffer;
}

class TestableQueueSourceNode : public AudioBufferQueueSourceNode {
 public:
  explicit TestableQueueSourceNode(const std::shared_ptr<BaseAudioContext> &context)
      : AudioBufferQueueSourceNode(context, BaseAudioBufferSourceOptions()) {}

  using AudioBufferQueueSourceNode::getCurrentPosition;
  using AudioBufferQueueSourceNode::processNode;
};

/// Recovers the concrete audio node owned by a graph node that was just added
/// via Graph::addNode (the unique_ptr payload lives inside the NodeHandle).
template <typename NodeT>
NodeT *nodeOf(utils::graph::HostGraph::Node *hostNode) {
  return static_cast<NodeT *>(hostNode->handle->audioNode->asAudioNode());
}

} // namespace

class AudioBufferQueueSourceTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        1, 5 * SAMPLE_RATE, SAMPLE_RATE, eventRegistry, RuntimeRegistry{});
    destination = std::make_shared<AudioDestinationNode>(context);
    context->initialize(destination.get());
  }

  /// Renders one quantum from `node` directly into its own output buffer and
  /// advances the context clock the way processGraph does in production
  /// (process first, then advance currentSampleFrame). The node is exercised
  /// in isolation here, so processGraph only serves to tick the clock.
  std::shared_ptr<DSPAudioBuffer> renderQuantum(TestableQueueSourceNode &node) {
    node.processNode(QUANTUM);
    auto clockBuffer = std::make_shared<DSPAudioBuffer>(QUANTUM, 1, (float)SAMPLE_RATE);
    context->processGraph(clockBuffer.get(), QUANTUM);
    return node.getOutputBuffer();
  }
};

/// Frame-consumption pace: rendering N quanta must consume exactly N * 128
/// source frames — the symptom in #1064 is consumption ~3x faster than
/// real time.
TEST_F(AudioBufferQueueSourceTest, ConsumesExactlyOneFramePerRenderedFrame) {
  TestableQueueSourceNode node(context);

  // 3 buffers x 1000 frames; 1000 % 128 != 0 exercises the cross-buffer path.
  constexpr size_t BUFFER_FRAMES = 1000;
  constexpr size_t BUFFER_COUNT = 3;
  for (size_t b = 0; b < BUFFER_COUNT; ++b) {
    node.enqueueBuffer(makeRampBuffer(BUFFER_FRAMES, b * BUFFER_FRAMES), b, nullptr);
  }

  node.start(0.0);

  constexpr size_t TOTAL_FRAMES = BUFFER_FRAMES * BUFFER_COUNT;
  size_t expected = 0;

  while (expected + QUANTUM <= TOTAL_FRAMES) {
    auto out = renderQuantum(node);
    ASSERT_NE(out, nullptr);
    auto channel = out->getChannel(0)->span();
    for (int i = 0; i < QUANTUM; ++i) {
      ASSERT_FLOAT_EQ(channel[i], static_cast<float>(expected + i) * 1e-6f)
          << "Discontinuity at global frame " << (expected + i)
          << " — source consumed frames at the wrong pace or emitted garbage";
    }
    expected += QUANTUM;
  }

  // Position must track rendered time 1:1 (the #1064 report measured ~3x).
  const double expectedPosition = static_cast<double>(expected) / SAMPLE_RATE;
  EXPECT_NEAR(node.getCurrentPosition(), expectedPosition, 2.0 * QUANTUM / (double)SAMPLE_RATE);
}

/// Streaming pattern from the #1064 report: chunks are enqueued while the
/// node is already playing (TTS over WebSocket). Content must stay gapless
/// and at 1x pace across enqueues that race buffer exhaustion.
TEST_F(AudioBufferQueueSourceTest, StreamingEnqueueKeepsPaceAndContinuity) {
  TestableQueueSourceNode node(context);

  constexpr size_t CHUNK_FRAMES = 10080; // 420ms @ 24kHz, matches the report
  size_t produced = 0;
  size_t chunkId = 0;

  node.enqueueBuffer(makeRampBuffer(CHUNK_FRAMES, produced), chunkId++, nullptr);
  produced += CHUNK_FRAMES;

  node.start(0.0);

  size_t expected = 0;
  // Render ~2.1s of audio, feeding a new chunk roughly every 0.42s of
  // rendered time, just like a realtime TTS stream would.
  constexpr size_t TOTAL_QUANTA = 400;
  for (size_t q = 0; q < TOTAL_QUANTA; ++q) {
    // Keep ~1 chunk of headroom queued, mirroring realtime arrival.
    if (expected + CHUNK_FRAMES > produced) {
      node.enqueueBuffer(makeRampBuffer(CHUNK_FRAMES, produced), chunkId++, nullptr);
      produced += CHUNK_FRAMES;
    }

    auto out = renderQuantum(node);
    ASSERT_NE(out, nullptr);
    auto channel = out->getChannel(0)->span();
    for (int i = 0; i < QUANTUM; ++i) {
      ASSERT_FLOAT_EQ(channel[i], static_cast<float>(expected + i) * 1e-6f)
          << "Discontinuity at global frame " << (expected + i) << " (quantum " << q << ")";
    }
    expected += QUANTUM;
  }

  const double expectedPosition = static_cast<double>(expected) / SAMPLE_RATE;
  EXPECT_NEAR(node.getCurrentPosition(), expectedPosition, 2.0 * QUANTUM / (double)SAMPLE_RATE);
}

/// Full-graph variant: a mono queue source added to the graph and connected
/// to the (stereo) destination, rendered through BaseAudioContext::processGraph
/// — i.e. the exact production pull path including graph pre-processing,
/// channel up-mixing and the destination's per-quantum normalize(). Content is
/// an impulse train (1.0 every 1000 frames); after normalization the impulse
/// POSITIONS must still land every 1000 output frames if consumption pace is
/// exactly 1x, and every other sample must stay silent.
TEST_F(AudioBufferQueueSourceTest, FullGraphRenderPreservesPaceAndSilence) {
  auto stereoContext = std::make_shared<OfflineAudioContext>(
      2, 5 * SAMPLE_RATE, SAMPLE_RATE, eventRegistry, RuntimeRegistry{});

  auto graph = stereoContext->getGraph();

  // The destination must live in the graph so processGraph() renders through
  // it and copies its output buffer back.
  auto *destinationHostNode = graph->addNode(std::make_unique<AudioDestinationNode>(stereoContext));
  auto *destinationNode = nodeOf<AudioDestinationNode>(destinationHostNode);
  stereoContext->initialize(destinationNode);

  // Mono queue source -> stereo destination, exactly like the production graph.
  auto *sourceHostNode = graph->addNode(
      std::make_unique<AudioBufferQueueSourceNode>(stereoContext, BaseAudioBufferSourceOptions()));
  auto *node = nodeOf<AudioBufferQueueSourceNode>(sourceHostNode);

  ASSERT_TRUE(graph->addEdge(sourceHostNode, destinationHostNode).is_ok());

  constexpr size_t CHUNK_FRAMES = 10080;
  constexpr size_t IMPULSE_PERIOD = 1000;
  size_t produced = 0;
  size_t chunkId = 0;

  auto makeImpulseChunk = [&]() {
    auto buffer = std::make_shared<AudioBuffer>(CHUNK_FRAMES, 1, (float)SAMPLE_RATE);
    auto channel = buffer->getChannel(0)->span();
    for (size_t i = 0; i < CHUNK_FRAMES; ++i) {
      channel[i] = ((produced + i) % IMPULSE_PERIOD == 0) ? 1.0f : 0.0f;
    }
    node->enqueueBuffer(buffer, chunkId++, nullptr);
    produced += CHUNK_FRAMES;
  };

  makeImpulseChunk();
  node->start(0.0);

  size_t rendered = 0;
  constexpr size_t TOTAL_QUANTA = 400;
  for (size_t q = 0; q < TOTAL_QUANTA; ++q) {
    if (rendered + CHUNK_FRAMES > produced) {
      makeImpulseChunk();
    }

    auto out = std::make_shared<DSPAudioBuffer>(QUANTUM, 2, (float)SAMPLE_RATE);
    stereoContext->processGraph(out.get(), QUANTUM);

    for (size_t ch = 0; ch < 2; ++ch) {
      auto channel = out->getChannel(ch)->span();
      for (int i = 0; i < QUANTUM; ++i) {
        const size_t globalFrame = rendered + i;
        if (globalFrame % IMPULSE_PERIOD == 0) {
          ASSERT_GT(channel[i], 0.5f) << "Missing impulse at frame " << globalFrame << " ch " << ch
                                      << " — source consumed frames at the wrong pace";
        } else {
          ASSERT_NEAR(channel[i], 0.0f, 1e-6f)
              << "Garbage at frame " << globalFrame << " ch " << ch;
        }
      }
    }
    rendered += QUANTUM;
  }
}

// NOLINTEND
