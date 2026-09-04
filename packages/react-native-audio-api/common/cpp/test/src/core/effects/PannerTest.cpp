#include <audioapi/core/AudioListener.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/PannerNode.h>
#include <audioapi/core/effects/PannerSpatialization.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>

using namespace audioapi;
using namespace audioapi::panner;

// NOLINTBEGIN

class PannerTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;
  std::shared_ptr<AudioListener> listener;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * sampleRate, sampleRate, eventRegistry);
    destination = std::make_shared<AudioDestinationNode>(context);
    context->initialize(destination.get());
    listener = std::make_shared<AudioListener>(context);
  }
};

class TestablePannerNode : public PannerNode {
 public:
  TestablePannerNode(const std::shared_ptr<BaseAudioContext> &context, AudioListener *listener)
      : PannerNode(context, listener, PannerOptions()) {}

  void setInputBuffer(const std::shared_ptr<DSPAudioBuffer> &input) {
    audioBuffer_ = input;
  }

  using PannerNode::processNode;
};

TEST_F(PannerTest, PannerCanBeCreated) {
  auto panner = std::make_shared<PannerNode>(context, listener.get(), PannerOptions());
  ASSERT_NE(panner, nullptr);
}

struct PannerTestParams {
  Vec3 sourcePosition;
  Vec3 sourceOrientation;
  Vec3 listenerPosition;
  Vec3 listenerForward;
  Vec3 listenerUp;
  float expectedGainL;
  float expectedGainR;
};

class PannerMonoParametrizedTest : public PannerTest,
                                   public testing::WithParamInterface<PannerTestParams> {};

TEST_P(PannerMonoParametrizedTest, PanModulatesInputMonoCorrectly) {
  const PannerTestParams &params = GetParam();

  static constexpr int FRAMES_TO_PROCESS = 4;
  TestablePannerNode panNode(context, listener.get());

  listener->getPositionXParam()->setValue(params.listenerPosition.x);
  listener->getPositionYParam()->setValue(params.listenerPosition.y);
  listener->getPositionZParam()->setValue(params.listenerPosition.z);

  listener->getForwardXParam()->setValue(params.listenerForward.x);
  listener->getForwardYParam()->setValue(params.listenerForward.y);
  listener->getForwardZParam()->setValue(params.listenerForward.z);

  listener->getUpXParam()->setValue(params.listenerUp.x);
  listener->getUpYParam()->setValue(params.listenerUp.y);
  listener->getUpZParam()->setValue(params.listenerUp.z);

  panNode.getPositionXParam()->setValue(params.sourcePosition.x);
  panNode.getPositionYParam()->setValue(params.sourcePosition.y);
  panNode.getPositionZParam()->setValue(params.sourcePosition.z);

  panNode.getOrientationXParam()->setValue(params.sourceOrientation.x);
  panNode.getOrientationYParam()->setValue(params.sourceOrientation.y);
  panNode.getOrientationZParam()->setValue(params.sourceOrientation.z);

  auto monoInputBuffer = std::make_shared<DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);

  for (size_t i = 0; i < monoInputBuffer->getSize(); ++i) {
    (*monoInputBuffer->getChannelByType(AudioBuffer::ChannelMono))[i] = i + 1;
  }

  panNode.setInputBuffer(monoInputBuffer);
  panNode.processNode(FRAMES_TO_PROCESS);

  auto resultBuffer = panNode.getOutputBuffer();

  const float distance = panner::computeDistance(params.sourcePosition, params.listenerPosition);
  const float distanceGain = panner::computeDistanceGain(
      panNode.getDistanceModel(),
      distance,
      panNode.getRefDistance(),
      panNode.getMaxDistance(),
      panNode.getRolloffFactor());
  const float coneGain = panner::computeConeGain(
      params.sourcePosition,
      params.listenerPosition,
      params.sourceOrientation,
      panNode.getConeInnerAngle(),
      panNode.getConeOuterAngle(),
      panNode.getConeOuterGain());
  const float totalGain = distanceGain * coneGain;

  constexpr float TOLERANCE = 1e-5f;

  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i],
        (i + 1) * params.expectedGainL * totalGain,
        TOLERANCE);
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[i],
        (i + 1) * params.expectedGainR * totalGain,
        TOLERANCE);
  }
}

INSTANTIATE_TEST_SUITE_P(
    PanModulatesInputMonoCorrectly,
    PannerMonoParametrizedTest,
    testing::Values(
        PannerTestParams{
            {1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f},
            0.0f,
            1.0f},

        PannerTestParams{
            {0.0f, 0.0f, -1.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f},
            0.707107f,
            0.707107f},

        PannerTestParams{
            {-1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f},
            1.0f,
            0.0f}));

class PannerStereoParametrizedTest : public PannerTest,
                                     public testing::WithParamInterface<PannerTestParams> {};

TEST_P(PannerStereoParametrizedTest, PanModulatesInputStereoCorrectly) {
  const PannerTestParams &params = GetParam();

  static constexpr int FRAMES_TO_PROCESS = 4;
  TestablePannerNode panNode(context, listener.get());

  listener->getPositionXParam()->setValue(params.listenerPosition.x);
  listener->getPositionYParam()->setValue(params.listenerPosition.y);
  listener->getPositionZParam()->setValue(params.listenerPosition.z);

  listener->getForwardXParam()->setValue(params.listenerForward.x);
  listener->getForwardYParam()->setValue(params.listenerForward.y);
  listener->getForwardZParam()->setValue(params.listenerForward.z);

  listener->getUpXParam()->setValue(params.listenerUp.x);
  listener->getUpYParam()->setValue(params.listenerUp.y);
  listener->getUpZParam()->setValue(params.listenerUp.z);

  panNode.getPositionXParam()->setValue(params.sourcePosition.x);
  panNode.getPositionYParam()->setValue(params.sourcePosition.y);
  panNode.getPositionZParam()->setValue(params.sourcePosition.z);

  panNode.getOrientationXParam()->setValue(params.sourceOrientation.x);
  panNode.getOrientationYParam()->setValue(params.sourceOrientation.y);
  panNode.getOrientationZParam()->setValue(params.sourceOrientation.z);

  auto inputBuffer = std::make_shared<DSPAudioBuffer>(FRAMES_TO_PROCESS, 2, sampleRate);
  for (size_t i = 0; i < inputBuffer->getSize(); ++i) {
    (*inputBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i] = i + 1;
    (*inputBuffer->getChannelByType(AudioBuffer::ChannelRight))[i] = i + 1;
  }
  const auto inputLeftSpan = inputBuffer->getChannelByType(AudioBuffer::ChannelLeft)->span();
  const auto inputRightSpan = inputBuffer->getChannelByType(AudioBuffer::ChannelRight)->span();

  panNode.setInputBuffer(inputBuffer);
  panNode.processNode(FRAMES_TO_PROCESS);

  auto resultBuffer = panNode.getOutputBuffer();

  float azimuth = panner::computeAzimuth(
      params.sourcePosition, params.listenerPosition, params.listenerForward, params.listenerUp);
  azimuth = panner::clampAzimuth(azimuth);
  azimuth = panner::wrapAzimuth(azimuth);

  const float distance = panner::computeDistance(params.sourcePosition, params.listenerPosition);
  const float distanceGain = panner::computeDistanceGain(
      panNode.getDistanceModel(),
      distance,
      panNode.getRefDistance(),
      panNode.getMaxDistance(),
      panNode.getRolloffFactor());
  const float coneGain = panner::computeConeGain(
      params.sourcePosition,
      params.listenerPosition,
      params.sourceOrientation,
      panNode.getConeInnerAngle(),
      panNode.getConeOuterAngle(),
      panNode.getConeOuterGain());
  const float totalGain = distanceGain * coneGain;

  constexpr float TOLERANCE = 1e-5f;

  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    if (azimuth <= 0) {
      EXPECT_NEAR(
          (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i],
          ((i + 1) + (i + 1) * params.expectedGainL) * totalGain,
          TOLERANCE);
      EXPECT_NEAR(
          (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[i],
          ((i + 1) * params.expectedGainR) * totalGain,
          TOLERANCE);
    } else {
      EXPECT_NEAR(
          (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i],
          ((i + 1) * params.expectedGainL) * totalGain,
          TOLERANCE);
      EXPECT_NEAR(
          (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[i],
          ((i + 1) + (i + 1) * params.expectedGainR) * totalGain,
          TOLERANCE);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    StereoPanningPositions,
    PannerStereoParametrizedTest,
    testing::Values(
        PannerTestParams{
            {1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f},
            0.0f,
            1.0f},
        PannerTestParams{
            {0.0f, 0.0f, -1.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f},
            0.0f,
            1.0f},
        PannerTestParams{
            {-1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f},
            1.0f,
            0.0f}));

TEST_F(PannerTest, PositionXAudioRateProcessing) {
  static constexpr int FRAMES_TO_PROCESS = 4;
  TestablePannerNode panNode(context, listener.get());

  auto monoInputBuffer = std::make_shared<DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < monoInputBuffer->getSize(); ++i) {
    (*monoInputBuffer->getChannelByType(AudioBuffer::ChannelMono))[i] = 1.0f;
  }

  // Test only the change in positionX parameter
  panNode.setRolloffFactor(0.0f);
  panNode.getPositionZParam()->setValue(-1.0f);

  panNode.setInputBuffer(monoInputBuffer);

  auto posXparam = panNode.getPositionXParam();
  const double startTime = context->getCurrentTime();
  const double endTime = startTime + (static_cast<double>(FRAMES_TO_PROCESS) / sampleRate);

  posXparam->setValueAtTime(-1.0f, startTime);
  posXparam->linearRampToValueAtTime(1.0f, endTime);

  panNode.processNode(FRAMES_TO_PROCESS);

  auto resultBuffer = panNode.getOutputBuffer();
  const auto outLeft = resultBuffer->getChannelByType(AudioBuffer::ChannelLeft)->span();
  const auto outRight = resultBuffer->getChannelByType(AudioBuffer::ChannelRight)->span();

  EXPECT_GT(outLeft[0], outLeft[1]);
  EXPECT_GT(outLeft[1], outLeft[2]);
  EXPECT_GT(outLeft[2], outLeft[3]);

  EXPECT_LT(outRight[0], outRight[1]);
  EXPECT_LT(outRight[1], outRight[2]);
  EXPECT_LT(outRight[2], outRight[3]);
}

TEST_F(PannerTest, ConeAngles) {
  static constexpr int FRAMES_TO_PROCESS = 4;
  TestablePannerNode panNode(context, listener.get());
  panNode.setConeInnerAngle(60);
  panNode.setConeOuterAngle(90);
  panNode.setConeOuterGain(0.0f);

  auto monoInputBuffer = std::make_shared<DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < monoInputBuffer->getSize(); ++i) {
    (*monoInputBuffer->getChannelByType(AudioBuffer::ChannelMono))[i] = 1.0f;
  }

  panNode.setInputBuffer(monoInputBuffer);

  constexpr float POSITION_Z_INFRONT = -1.0f;
  // The source is facing backwards, sound should not be heard by the listener
  constexpr float ORIENTATION_Z_BEHIND = -1.0f;
  panNode.getPositionZParam()->setValue(POSITION_Z_INFRONT);
  panNode.getOrientationXParam()->setValue(0.0f);
  panNode.getOrientationYParam()->setValue(0.0f);
  panNode.getOrientationZParam()->setValue(ORIENTATION_Z_BEHIND);

  panNode.processNode(FRAMES_TO_PROCESS);

  auto resultBuffer = panNode.getOutputBuffer();

  constexpr float TOLERANCE = 1e-5f;
  constexpr float EXPECTED_SILENCE = 0.0f;

  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i],
        EXPECTED_SILENCE,
        TOLERANCE);
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[i],
        EXPECTED_SILENCE,
        TOLERANCE);
  }
}

// NOLINTEND
