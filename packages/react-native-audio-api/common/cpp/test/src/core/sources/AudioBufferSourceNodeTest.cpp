#include <audioapi/core/AudioParam.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/dsp/WsolaTimeStretcher.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr int SAMPLE_RATE = 24000;
constexpr int QUANTUM = RENDER_QUANTUM_SIZE;
constexpr float MODULATION = 1.0f; // playbackRate.value (1) + modulation → effective rate 2
constexpr size_t NUM_QUANTA = 20;

std::shared_ptr<AudioBuffer> makeSilentBuffer(size_t frames) {
  return std::make_shared<AudioBuffer>(frames, 1, static_cast<float>(SAMPLE_RATE));
}

class TestableAudioBufferSourceNode : public AudioBufferSourceNode {
 public:
  explicit TestableAudioBufferSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioBufferSourceOptions &options)
      : AudioBufferSourceNode(context, options) {}

  using AudioBufferSourceNode::getCurrentPosition;
  using AudioBufferSourceNode::initStretch;
  using AudioBufferSourceNode::isEmpty;
  using AudioBufferSourceNode::setBuffer;
};

/// Recovers the concrete audio node owned by a graph node that was just added
/// via Graph::addNode (the unique_ptr payload lives inside the NodeHandle).
template <typename NodeT>
NodeT *nodeOf(utils::graph::HostGraph::Node *hostNode) {
  return static_cast<NodeT *>(hostNode->handle->audioNode->asAudioNode());
}

} // namespace

class AudioBufferSourceNodeTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  utils::graph::HostGraph::Node *destinationHostNode = nullptr;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(1, 5 * SAMPLE_RATE, SAMPLE_RATE, eventRegistry);
    auto destination = std::make_unique<AudioDestinationNode>(context);
    context->initialize(destination.get());
    destinationHostNode = context->getGraph()->addNode(std::move(destination));
  }

  /// Registers `node` in the context graph and connects it to the destination
  /// so `processGraph()` will pull through it. Returns the live node pointer
  /// (ownership lives in the graph).
  TestableAudioBufferSourceNode *addNode(std::unique_ptr<TestableAudioBufferSourceNode> node) {
    auto *hostNode = context->getGraph()->addNode(std::move(node));
    EXPECT_TRUE(context->getGraph()->addEdge(hostNode, destinationHostNode).is_ok());
    return nodeOf<TestableAudioBufferSourceNode>(hostNode);
  }

  /// Renders one quantum through the real graph pull path.
  void processGraph() {
    auto out = std::make_shared<DSPAudioBuffer>(QUANTUM, 1, static_cast<float>(SAMPLE_RATE));
    context->processGraph(out.get(), QUANTUM);
  }

  void fillPlaybackRateModulation(TestableAudioBufferSourceNode &node, float modulation) {
    auto inputBuffer = node.getPlaybackRateParam()->getInputBuffer();
    ASSERT_NE(inputBuffer, nullptr);
    auto channel = inputBuffer->getChannel(0)->span();
    ASSERT_GE(channel.size(), 1u);
    channel[0] = modulation;
  }

  /// Renders one quantum after refilling BridgeNode-style modulation on playbackRate.
  void renderQuantumWithModulation(TestableAudioBufferSourceNode &node, float modulation) {
    fillPlaybackRateModulation(node, modulation);
    processGraph();
  }

  std::unique_ptr<TestableAudioBufferSourceNode> makeNode(bool pitchCorrection) {
    BaseAudioBufferSourceOptions baseOptions;
    baseOptions.pitchCorrection = pitchCorrection;
    baseOptions.playbackRate = 1.0f;
    baseOptions.detune = 0.0f;
    AudioBufferSourceOptions options(baseOptions);

    auto node = std::make_unique<TestableAudioBufferSourceNode>(context, options);

    // Enough source for NUM_QUANTA at 2x plus WSOLA tail padding (mirrors HostObject setBuffer).
    const size_t extraTailFrames = static_cast<size_t>(
        (WsolaTimeStretcher::INPUT_LATENCY_MS + WsolaTimeStretcher::OUTPUT_LATENCY_MS) / 1000.0f *
        SAMPLE_RATE);
    const size_t bufferFrames = static_cast<size_t>(NUM_QUANTA * QUANTUM * 4) + extraTailFrames;
    auto buffer = makeSilentBuffer(bufferFrames);
    auto outputBuffer =
        std::make_shared<DSPAudioBuffer>(QUANTUM, 1, static_cast<float>(SAMPLE_RATE));

    if (pitchCorrection) {
      auto playbackRateBuffer = std::make_shared<DSPAudioBuffer>(
          static_cast<size_t>(WsolaTimeStretcher::MAX_PLAYBACK_RATE * QUANTUM),
          1,
          static_cast<float>(SAMPLE_RATE));
      node->initStretch(1, static_cast<float>(SAMPLE_RATE), playbackRateBuffer);
    }

    node->setBuffer(buffer, outputBuffer);
    node->start(0.0);
    return node;
  }

  std::unique_ptr<TestableAudioBufferSourceNode> makeNodeWithoutBuffer() {
    return std::make_unique<TestableAudioBufferSourceNode>(
        context, AudioBufferSourceOptions(BaseAudioBufferSourceOptions()));
  }

  std::shared_ptr<DSPAudioBuffer> makeOutputBuffer() {
    return std::make_shared<DSPAudioBuffer>(QUANTUM, 1, static_cast<float>(SAMPLE_RATE));
  }
};

/// With pitchCorrection on, processNode() used to call processKRateParam twice per
/// quantum (gate check, then the stretch branch). The first call zeros inputBuffer_,
/// so BridgeNode modulation was dropped and playback ran at .value (1x) instead of the
/// modulated rate. Position must advance at ~2x when modulation = +1 on a 1.0 base rate.
TEST_F(
    AudioBufferSourceNodeTest,
    PitchCorrectionKeepsPlaybackRateModulationAcrossProcessKRateCalls) {
  auto *node = addNode(makeNode(/*pitchCorrection=*/true));

  for (size_t q = 0; q < NUM_QUANTA; ++q) {
    renderQuantumWithModulation(*node, MODULATION);
  }

  constexpr double RENDERED_SECONDS =
      static_cast<double>(NUM_QUANTA * QUANTUM) / static_cast<double>(SAMPLE_RATE);
  constexpr double EXPECTED_POSITION = 2.0 * RENDERED_SECONDS;
  constexpr double TOLERANCE = 2.0 * QUANTUM / static_cast<double>(SAMPLE_RATE);

  EXPECT_NEAR(node->getCurrentPosition(), EXPECTED_POSITION, TOLERANCE)
      << "Expected ~2x source consumption when pitchCorrection is on and playbackRate "
         "is modulated via inputBuffer_ (BridgeNode path). ~1x means the first "
         "processKRateParam call zeroed modulation before the stretch branch read it.";
}

/// Control: without pitchCorrection the gate short-circuits, so processKRateParam runs
/// once and modulation is applied. This must pass even on the buggy mainline path.
TEST_F(AudioBufferSourceNodeTest, WithoutPitchCorrectionPlaybackRateModulationAdvancesAt2x) {
  auto *node = addNode(makeNode(/*pitchCorrection=*/false));

  for (size_t q = 0; q < NUM_QUANTA; ++q) {
    renderQuantumWithModulation(*node, MODULATION);
  }

  constexpr double RENDERED_SECONDS =
      static_cast<double>(NUM_QUANTA * QUANTUM) / static_cast<double>(SAMPLE_RATE);
  constexpr double EXPECTED_POSITION = 2.0 * RENDERED_SECONDS;
  constexpr double TOLERANCE = 2.0 * QUANTUM / static_cast<double>(SAMPLE_RATE);

  EXPECT_NEAR(node->getCurrentPosition(), EXPECTED_POSITION, TOLERANCE);
}

/// Spec ("Playback of AudioBuffer Contents"): a null buffer forces the stop time to
/// currentTime, so a started source with no buffer ends in the first rendered quantum,
/// not when `when` arrives. Without this, processNode() bails on the empty buffer
/// forever and `ended` never fires.
TEST_F(AudioBufferSourceNodeTest, StartedWithNullBufferEndsOnFirstRenderInsteadOfAtStartTime) {
  static constexpr uint64_t ENDED_CALLBACK_ID = 51;
  static constexpr double START_BEYOND_RENDERED_RANGE = 4.0;

  auto *node = addNode(makeNodeWithoutBuffer());
  node->assignOnEndedCallbackId(ENDED_CALLBACK_ID);
  EXPECT_CALL(*eventRegistry, unregisterHandler(AudioEvent::ENDED, ENDED_CALLBACK_ID))
      .Times(testing::AnyNumber());
  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(testing::_, AudioEvent::ENDED, ENDED_CALLBACK_ID, testing::_))
      .WillOnce(testing::Return(true));

  node->start(START_BEYOND_RENDERED_RANGE, /*offset=*/0.0);
  EXPECT_TRUE(node->isScheduled());

  processGraph();
  EXPECT_TRUE(node->isFinished());

  processGraph(); // `ended` must not fire a second time
}

/// The setter acquires a buffer assigned after start(), so content that lands before
/// the first render is played (WPT audiobuffersource-grain "set buffer after start").
TEST_F(AudioBufferSourceNodeTest, BufferAssignedAfterStartButBeforeFirstRenderIsPlayed) {
  static constexpr uint64_t ENDED_CALLBACK_ID = 52;

  auto *node = addNode(makeNodeWithoutBuffer());
  node->assignOnEndedCallbackId(ENDED_CALLBACK_ID);
  EXPECT_CALL(*eventRegistry, unregisterHandler(AudioEvent::ENDED, ENDED_CALLBACK_ID))
      .Times(testing::AnyNumber());
  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(testing::_, AudioEvent::ENDED, ENDED_CALLBACK_ID, testing::_))
      .Times(0);

  node->start(/*when=*/0.0, /*offset=*/0.0);
  node->setBuffer(makeSilentBuffer(NUM_QUANTA * QUANTUM), makeOutputBuffer());

  processGraph();
  EXPECT_TRUE(node->isPlaying());
  EXPECT_FALSE(node->isEmpty());
}

/// Once a null-buffer source has ended, a buffer assigned later must be dropped instead
/// of resurrecting the node into playing (and ending a second time) at `when`.
TEST_F(AudioBufferSourceNodeTest, BufferAssignedAfterNullBufferSourceEndedIsIgnored) {
  static constexpr uint64_t ENDED_CALLBACK_ID = 53;

  auto *node = addNode(makeNodeWithoutBuffer());
  node->assignOnEndedCallbackId(ENDED_CALLBACK_ID);
  EXPECT_CALL(*eventRegistry, unregisterHandler(AudioEvent::ENDED, ENDED_CALLBACK_ID))
      .Times(testing::AnyNumber());
  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(testing::_, AudioEvent::ENDED, ENDED_CALLBACK_ID, testing::_))
      .WillOnce(testing::Return(true));

  node->start(/*when=*/0.0, /*offset=*/0.0);
  processGraph();
  EXPECT_TRUE(node->isFinished());

  node->setBuffer(makeSilentBuffer(NUM_QUANTA * QUANTUM), makeOutputBuffer());
  EXPECT_TRUE(node->isEmpty());

  for (size_t q = 0; q < NUM_QUANTA; ++q) {
    processGraph();
  }

  EXPECT_TRUE(node->isFinished());
  EXPECT_TRUE(node->isEmpty());
}

// NOLINTEND
