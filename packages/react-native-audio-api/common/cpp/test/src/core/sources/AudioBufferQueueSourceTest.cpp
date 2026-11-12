#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>
#include <span>
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
NodeT *nodeOf(utils::graph::HostGraph::HostVertex *hostNode) {
  return static_cast<NodeT *>(hostNode->handle->audioNode->asAudioNode());
}

/// A mono queue source wired to the destination of its own context, rendered
/// through processGraph like production. Used by the pause/resume tests.
struct GraphQueueSource {
  std::shared_ptr<OfflineAudioContext> context;
  /// Non-owning: the node lives in `context`'s graph, which outlives this struct's use.
  AudioBufferQueueSourceNode *node;
  std::shared_ptr<DSPAudioBuffer> out;

  std::span<float> render() {
    context->processGraph(out.get(), QUANTUM);
    return out->getChannel(0)->span();
  }
};

GraphQueueSource makeGraphQueueSource(
    const std::shared_ptr<MockAudioEventHandlerRegistry> &eventRegistry,
    size_t frames = SAMPLE_RATE) {
  auto context =
      std::make_shared<OfflineAudioContext>(1, 5 * SAMPLE_RATE, SAMPLE_RATE, eventRegistry);
  auto graph = context->getGraph();

  auto *destinationHostNode = graph->addNode(std::make_unique<AudioDestinationNode>(context));
  context->initialize(nodeOf<AudioDestinationNode>(destinationHostNode));

  auto *sourceHostNode = graph->addNode(
      std::make_unique<AudioBufferQueueSourceNode>(context, BaseAudioBufferSourceOptions()));
  EXPECT_TRUE(graph->addEdge(sourceHostNode, destinationHostNode).is_ok());

  auto *node = nodeOf<AudioBufferQueueSourceNode>(sourceHostNode);
  if (frames > 0) {
    // Starts at 1 so a played frame is never 0.0f, which marks silence below.
    node->enqueueBuffer(makeRampBuffer(frames, 1), 0, nullptr);
  }
  return {context, node, std::make_shared<DSPAudioBuffer>(QUANTUM, 1, (float)SAMPLE_RATE)};
}

} // namespace

class AudioBufferQueueSourceTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(1, 5 * SAMPLE_RATE, SAMPLE_RATE, eventRegistry);
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
/// — i.e. the exact production pull path including graph pre-processing and
/// channel up-mixing. Content is an impulse train (1.0 every 1000 frames); the
/// impulse POSITIONS must land every 1000 output frames if consumption pace is
/// exactly 1x, and every other sample must stay silent.
TEST_F(AudioBufferQueueSourceTest, FullGraphRenderPreservesPaceAndSilence) {
  auto stereoContext =
      std::make_shared<OfflineAudioContext>(2, 5 * SAMPLE_RATE, SAMPLE_RATE, eventRegistry);

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

/// pause() then resume() on a playing node: the paused node stays in the graph,
/// is silent, fires no ended event, and continues from the frame it stopped at.
TEST_F(AudioBufferQueueSourceTest, PauseThenResumeContinuesFromPausedFrame) {
  static constexpr uint64_t ENDED_CALLBACK_ID = 42;
  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(testing::_, AudioEvent::ENDED, ENDED_CALLBACK_ID, testing::_))
      .Times(0);
  EXPECT_CALL(*eventRegistry, dispatchEvent(AudioEvent::ENDED, ENDED_CALLBACK_ID, testing::_))
      .Times(0);
  EXPECT_CALL(*eventRegistry, unregisterHandler(AudioEvent::ENDED, ENDED_CALLBACK_ID))
      .Times(testing::AnyNumber());

  auto source = makeGraphQueueSource(eventRegistry);
  source.node->assignOnEndedCallbackId(ENDED_CALLBACK_ID);
  source.node->start(0.0);

  float lastPlayed = 0.0f;
  for (int q = 0; q < 4; ++q) {
    lastPlayed = source.render()[QUANTUM - 1];
  }
  ASSERT_GT(lastPlayed, 0.0f);

  source.node->pausePlayback();
  for (int q = 0; q < 3; ++q) {
    auto channel = source.render();
    for (int i = 0; i < QUANTUM; ++i) {
      ASSERT_NEAR(channel[i], 0.0f, 1e-9f) << "Paused node produced audio at frame " << i;
    }
  }

  source.node->resumePlayback(0.0);
  auto channel = source.render();
  ASSERT_NEAR(channel[0], lastPlayed + 1e-6f, 1e-9f)
      << "Resumed node did not continue from the frame it was paused at";
  for (int i = 1; i < QUANTUM; ++i) {
    ASSERT_NEAR(channel[i], channel[i - 1] + 1e-6f, 1e-9f) << "Gap after resume at frame " << i;
  }
}

/// pause() before the first rendered quantum (JS allows it right after
/// start()): resume() plays the queue from its first frame.
TEST_F(AudioBufferQueueSourceTest, PauseBeforeFirstQuantumThenResumePlaysFromStart) {
  auto source = makeGraphQueueSource(eventRegistry);
  source.node->start(0.0);
  source.node->pausePlayback();

  for (int q = 0; q < 2; ++q) {
    auto channel = source.render();
    for (int i = 0; i < QUANTUM; ++i) {
      ASSERT_NEAR(channel[i], 0.0f, 1e-9f) << "Paused node produced audio at frame " << i;
    }
  }

  source.node->resumePlayback(0.0);
  auto channel = source.render();
  ASSERT_NEAR(channel[0], 1e-6f, 1e-9f) << "Resumed node did not start at the first frame";
}

/// stop() on a paused node, whether the pause has been rendered yet or not: the
/// node stays silent and fires ended exactly once, by its stop time.
TEST_F(AudioBufferQueueSourceTest, StopWhilePausedStaysSilentAndEndsOnce) {
  struct Case {
    const char *name;
    uint64_t endedCallbackId;
    bool isStartedBeforePause;
    int quantaBetweenPauseAndStop;
    double stopTimeAfterNow;
    size_t queuedFrames = SAMPLE_RATE;
  };
  const std::vector<Case> cases = {
      {"stop(0) after the pause rendered", 51, true, 2, 0.0},
      {"stop(later) in the same batch as pause()", 52, true, 0, 1.0},
      {"stop() inside the next quantum after the pause rendered",
       53,
       true,
       2,
       0.5 * QUANTUM / SAMPLE_RATE},
      {"stop(0) after pause() before the first quantum", 54, false, 1, 0.0},
      {"stop(0) after pausing a queue that played all its buffers", 55, true, 2, 0.0, QUANTUM},
      {"stop(0) after pausing a queue that never had a buffer", 56, false, 2, 0.0, 0},
  };

  EXPECT_CALL(*eventRegistry, unregisterHandler(AudioEvent::ENDED, testing::_))
      .Times(testing::AnyNumber());
  for (const auto &testCase : cases) {
    EXPECT_CALL(
        *eventRegistry,
        dispatchEventFromAudioThread(
            testing::_, AudioEvent::ENDED, testCase.endedCallbackId, testing::_))
        .Times(1)
        .WillOnce(testing::Return(true));
  }

  for (const auto &testCase : cases) {
    SCOPED_TRACE(testCase.name);
    auto source = makeGraphQueueSource(eventRegistry, testCase.queuedFrames);
    source.node->assignOnEndedCallbackId(testCase.endedCallbackId);
    source.node->start(0.0);
    if (testCase.isStartedBeforePause) {
      for (int q = 0; q < 2; ++q) {
        auto channel = source.render();
        if (testCase.queuedFrames >= 2 * QUANTUM) {
          ASSERT_GT(channel[QUANTUM - 1], 0.0f);
        }
      }
    }

    source.node->pausePlayback();
    for (int q = 0; q < testCase.quantaBetweenPauseAndStop; ++q) {
      source.render();
    }
    source.node->stop(source.context->getCurrentTime() + testCase.stopTimeAfterNow);
    // JS does not guard a second stop(); it must not end the node again.
    source.node->stop(0.0);

    const int quantaUntilStop =
        static_cast<int>(testCase.stopTimeAfterNow * SAMPLE_RATE / QUANTUM) + 3;
    for (int q = 0; q < quantaUntilStop; ++q) {
      auto channel = source.render();
      for (int i = 0; i < QUANTUM; ++i) {
        ASSERT_NEAR(channel[i], 0.0f, 1e-9f) << "Stopped paused node produced audio at frame " << i;
      }
    }
  }
}

namespace {

double quantaToTime(int quanta) {
  return static_cast<double>(quanta) * QUANTUM / SAMPLE_RATE;
}

void expectSilent(std::span<float> channel) {
  for (int i = 0; i < QUANTUM; ++i) {
    ASSERT_NEAR(channel[i], 0.0f, 1e-9f) << "Expected silence at frame " << i;
  }
}

} // namespace

/// A stop() time is absolute: resuming before it plays on until that time,
/// whether stop() was called before or during the pause.
TEST_F(AudioBufferQueueSourceTest, ResumeBeforeQueuedStopPlaysUntilStopTime) {
  constexpr int STOP_QUANTUM = 10;
  constexpr int PAUSE_QUANTUM = 2;
  constexpr int RESUME_QUANTUM = 5;

  for (const bool isStoppedBeforePause : {true, false}) {
    SCOPED_TRACE(isStoppedBeforePause ? "stop() then pause()" : "pause() then stop()");
    const uint64_t endedCallbackId = isStoppedBeforePause ? 61 : 62;
    int endedCount = 0;
    EXPECT_CALL(
        *eventRegistry,
        dispatchEventFromAudioThread(testing::_, AudioEvent::ENDED, endedCallbackId, testing::_))
        .WillRepeatedly([&endedCount](auto &&...) {
          ++endedCount;
          return true;
        });
    EXPECT_CALL(*eventRegistry, unregisterHandler(AudioEvent::ENDED, endedCallbackId))
        .Times(testing::AnyNumber());

    auto source = makeGraphQueueSource(eventRegistry);
    source.node->assignOnEndedCallbackId(endedCallbackId);
    source.node->start(0.0);

    float lastPlayed = 0.0f;
    for (int q = 0; q < PAUSE_QUANTUM; ++q) {
      lastPlayed = source.render()[QUANTUM - 1];
    }
    if (isStoppedBeforePause) {
      source.node->stop(quantaToTime(STOP_QUANTUM));
      source.node->pausePlayback();
    } else {
      source.node->pausePlayback();
      source.node->stop(quantaToTime(STOP_QUANTUM));
    }

    for (int q = PAUSE_QUANTUM; q < RESUME_QUANTUM; ++q) {
      expectSilent(source.render());
    }
    EXPECT_FALSE(source.node->hasEnded()) << "a paused node reported itself ended";

    source.node->resumePlayback(0.0);
    for (int q = RESUME_QUANTUM; q < STOP_QUANTUM; ++q) {
      auto channel = source.render();
      ASSERT_NEAR(channel[0], lastPlayed + 1e-6f, 1e-9f)
          << "Resumed node is not continuous at quantum " << q;
      lastPlayed = channel[QUANTUM - 1];
    }
    EXPECT_EQ(endedCount, 0) << "ended fired before the stop time";
    EXPECT_FALSE(source.node->hasEnded());

    for (int q = STOP_QUANTUM; q < STOP_QUANTUM + 3; ++q) {
      expectSilent(source.render());
    }
    EXPECT_EQ(endedCount, 1);
    EXPECT_TRUE(source.node->hasEnded());
  }
}

/// A node still paused when its stop() time comes ends then, not earlier,
/// including when resume() was asked for a time after the stop.
TEST_F(AudioBufferQueueSourceTest, PausedNodeEndsAtQueuedStopTime) {
  constexpr int PAUSE_QUANTUM = 2;
  constexpr int STOP_QUANTUM = 6;

  struct Case {
    const char *name;
    uint64_t endedCallbackId;
    bool isResumedAfterStopTime;
  };
  for (const auto &testCase :
       {Case{"no resume()", 71, false}, Case{"resume() later than the stop", 72, true}}) {
    SCOPED_TRACE(testCase.name);
    int endedCount = 0;
    AudioBufferQueueSourceNode *endingNode = nullptr;
    bool hasEndedAtDispatch = false;
    EXPECT_CALL(
        *eventRegistry,
        dispatchEventFromAudioThread(
            testing::_, AudioEvent::ENDED, testCase.endedCallbackId, testing::_))
        .WillRepeatedly([&](auto &&...) {
          ++endedCount;
          hasEndedAtDispatch = endingNode->hasEnded();
          return true;
        });
    EXPECT_CALL(*eventRegistry, unregisterHandler(AudioEvent::ENDED, testCase.endedCallbackId))
        .Times(testing::AnyNumber());

    auto source = makeGraphQueueSource(eventRegistry);
    endingNode = source.node;
    source.node->assignOnEndedCallbackId(testCase.endedCallbackId);
    source.node->start(0.0);
    for (int q = 0; q < PAUSE_QUANTUM; ++q) {
      source.render();
    }

    source.node->pausePlayback();
    source.node->stop(quantaToTime(STOP_QUANTUM));
    if (testCase.isResumedAfterStopTime) {
      source.node->resumePlayback(quantaToTime(STOP_QUANTUM + 4));
    }

    for (int q = PAUSE_QUANTUM; q < STOP_QUANTUM; ++q) {
      expectSilent(source.render());
    }
    EXPECT_EQ(endedCount, 0) << "ended fired before the stop time";
    EXPECT_FALSE(source.node->hasEnded());

    for (int q = STOP_QUANTUM; q < STOP_QUANTUM + 8; ++q) {
      expectSilent(source.render());
    }
    EXPECT_EQ(endedCount, 1);
    EXPECT_TRUE(hasEndedAtDispatch) << "JS handling ended must already read hasEnded";

    // The node has ended: a late resume() must not bring it back.
    source.node->resumePlayback(0.0);
    expectSilent(source.render());
    EXPECT_EQ(endedCount, 1);
  }
}

// NOLINTEND
