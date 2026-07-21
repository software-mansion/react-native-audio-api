#include <audioapi/core/AudioParam.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/WsolaTimeStretcher.h>
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
  using AudioBufferSourceNode::processNode;
  using AudioBufferSourceNode::setBuffer;
};

} // namespace

class AudioBufferSourceNodeTest : public ::testing::Test {
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
    node.processNode(QUANTUM);
    auto clockBuffer =
        std::make_shared<DSPAudioBuffer>(QUANTUM, 1, static_cast<float>(SAMPLE_RATE));
    context->processGraph(clockBuffer.get(), QUANTUM);
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
};

/// With pitchCorrection on, processNode() used to call processKRateParam twice per
/// quantum (gate check, then the stretch branch). The first call zeros inputBuffer_,
/// so BridgeNode modulation was dropped and playback ran at .value (1x) instead of the
/// modulated rate. Position must advance at ~2x when modulation = +1 on a 1.0 base rate.
TEST_F(
    AudioBufferSourceNodeTest,
    PitchCorrectionKeepsPlaybackRateModulationAcrossProcessKRateCalls) {
  auto node = makeNode(/*pitchCorrection=*/true);

  for (size_t q = 0; q < NUM_QUANTA; ++q) {
    renderQuantumWithModulation(*node, MODULATION);
  }

  const double renderedSeconds =
      static_cast<double>(NUM_QUANTA * QUANTUM) / static_cast<double>(SAMPLE_RATE);
  const double expectedPosition = 2.0 * renderedSeconds;
  const double tolerance = 2.0 * QUANTUM / static_cast<double>(SAMPLE_RATE);

  EXPECT_NEAR(node->getCurrentPosition(), expectedPosition, tolerance)
      << "Expected ~2x source consumption when pitchCorrection is on and playbackRate "
         "is modulated via inputBuffer_ (BridgeNode path). ~1x means the first "
         "processKRateParam call zeroed modulation before the stretch branch read it.";
}

/// Control: without pitchCorrection the gate short-circuits, so processKRateParam runs
/// once and modulation is applied. This must pass even on the buggy mainline path.
TEST_F(AudioBufferSourceNodeTest, WithoutPitchCorrectionPlaybackRateModulationAdvancesAt2x) {
  auto node = makeNode(/*pitchCorrection=*/false);

  for (size_t q = 0; q < NUM_QUANTA; ++q) {
    renderQuantumWithModulation(*node, MODULATION);
  }

  const double renderedSeconds =
      static_cast<double>(NUM_QUANTA * QUANTUM) / static_cast<double>(SAMPLE_RATE);
  const double expectedPosition = 2.0 * renderedSeconds;
  const double tolerance = 2.0 * QUANTUM / static_cast<double>(SAMPLE_RATE);

  EXPECT_NEAR(node->getCurrentPosition(), expectedPosition, tolerance);
}

// NOLINTEND
