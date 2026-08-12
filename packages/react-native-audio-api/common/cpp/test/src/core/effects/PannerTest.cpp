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

TEST_F(PannerTest, PannerCanBeCreatedWithCone) {
  auto panner = std::make_shared<PannerNode>(context, listener.get(), PannerOptions());
  panner->setConeInnerAngle(60);
  panner->setConeOuterAngle(90);
  panner->setConeOuterGain(0);
  ASSERT_NE(panner, nullptr);
}

TEST_F(PannerTest, SampleTestPanModulatesInputMonoCorrectly) {
  static constexpr int FRAMES_TO_PROCESS = 4;
  TestablePannerNode panNode(context, listener.get());

  Vec3 sourcePosition = {1.0f, 0.0f, 0.0f};
  Vec3 listenerPosition = {0.0f, 0.0f, 0.0f};
  Vec3 listenerForward = {0.0f, 0.0f, -1.0f};
  Vec3 listenerUp = {0.0f, 1.0f, 0.0f};

  // In this case these three positions need setting
  panNode.getPositionXParam()->setValue(sourcePosition.x);
  listener->getForwardZParam()->setValue(listenerForward.z);
  listener->getUpYParam()->setValue(listenerUp.y);

  auto monoInputBuffer = std::make_shared<DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < monoInputBuffer->getSize(); ++i) {
    (*monoInputBuffer->getChannelByType(AudioBuffer::ChannelMono))[i] = i + 1;
  }

  panNode.setInputBuffer(monoInputBuffer);
  panNode.processNode(FRAMES_TO_PROCESS);

  constexpr float EXPECTED_GAIN_L = -4.37114e-08;
  constexpr float EXPECTED_GAIN_R = 1;

  auto resultBuffer = panNode.getOutputBuffer();

  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i],
        (i + 1) * EXPECTED_GAIN_L,
        1e-4);
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[i],
        (i + 1) * EXPECTED_GAIN_R,
        1e-4);
  }
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

class PannerParametrizedTest : public PannerTest,
                               public testing::WithParamInterface<PannerTestParams> {};

TEST_P(PannerParametrizedTest, PanModulatesInputMonoCorrectly) {
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

  constexpr float TOLERANCE = 1e-4f;

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
    PannerParametrizedTest,
    testing::Values(
        PannerTestParams{
            {1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f},
            -4.37114e-08f,
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
            -4.37114e-08f}));

TEST_F(PannerTest, SampleTestPanModulatesInputStereoCorrectly) {
  static constexpr int FRAMES_TO_PROCESS = 4;
  TestablePannerNode panNode(context, listener.get());

  Vec3 sourcePosition = {1.0f, 0.0f, 0.0f};
  Vec3 sourceOrientation = {1.0f, 0.0f, 0.0f};
  Vec3 listenerPosition = {0.0f, 0.0f, 0.0f};
  Vec3 listenerForward = {0.0f, 0.0f, -1.0f};
  Vec3 listenerUp = {0.0f, 1.0f, 0.0f};

  // In this case these three positions need setting
  panNode.getPositionXParam()->setValue(sourcePosition.x);
  panNode.getOrientationXParam()->setValue(sourceOrientation.x);
  panNode.getOrientationYParam()->setValue(sourceOrientation.y);
  panNode.getOrientationZParam()->setValue(sourceOrientation.z);

  listener->getForwardZParam()->setValue(listenerForward.z);
  listener->getUpYParam()->setValue(listenerUp.y);

  auto inputBuffer = std::make_shared<DSPAudioBuffer>(FRAMES_TO_PROCESS, 2, sampleRate);
  for (size_t i = 0; i < inputBuffer->getSize(); ++i) {
    (*inputBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i] = i + 1;
    (*inputBuffer->getChannelByType(AudioBuffer::ChannelRight))[i] = i + 1;
  }
  const auto inputLeftSpan = inputBuffer->getChannelByType(AudioBuffer::ChannelLeft)->span();
  const auto inputRightSpan = inputBuffer->getChannelByType(AudioBuffer::ChannelRight)->span();

  panNode.setInputBuffer(inputBuffer);
  panNode.processNode(FRAMES_TO_PROCESS);

  float azimuth =
      panner::computeAzimuth(sourcePosition, listenerPosition, listenerForward, listenerUp);
  azimuth = panner::clampAzimuth(azimuth);
  azimuth = panner::wrapAzimuth(azimuth);
  float x;
  if (azimuth <= 0) {
    x = (azimuth + 90) / 90;
  } else {
    x = azimuth / 90;
  }
  float gainL = std::cos(x * PI / 2.0f);
  float gainR = std::sin(x * PI / 2.0f);

  const float distance = panner::computeDistance(sourcePosition, listenerPosition);
  const float distanceGain = panner::computeDistanceGain(
      panNode.getDistanceModel(),
      distance,
      panNode.getRefDistance(),
      panNode.getMaxDistance(),
      panNode.getRolloffFactor());
  const float coneGain = panner::computeConeGain(
      sourcePosition,
      listenerPosition,
      sourceOrientation,
      panNode.getConeInnerAngle(),
      panNode.getConeOuterAngle(),
      panNode.getConeOuterGain());
  const float totalGain = distanceGain * coneGain;

  constexpr float EXPECTED_GAIN_L = 0.0f;
  constexpr float EXPECTED_GAIN_R = 1;

  constexpr float TOLERANCE = 1e-5f;

  auto resultBuffer = panNode.getOutputBuffer();

  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {

    if (azimuth <= 0) {
      EXPECT_NEAR(
          (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i],
          ((i + 1) + (i + 1) * EXPECTED_GAIN_L) * totalGain,
          TOLERANCE);
      EXPECT_NEAR(
          (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[i],
          ((i + 1) * EXPECTED_GAIN_R) * totalGain,
          TOLERANCE);
    } else {
      EXPECT_NEAR(
          (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i],
          ((i + 1) * EXPECTED_GAIN_L) * totalGain,
          TOLERANCE);
      EXPECT_NEAR(
          (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[i],
          ((i + 1) + (i + 1) * EXPECTED_GAIN_R) * totalGain,
          TOLERANCE);
    }
  }
}

TEST_F(PannerTest, MonoSourcePannedByPosition) {
  static constexpr int FRAMES_TO_PROCESS = 4;
  TestablePannerNode panNode(context, listener.get());
  panNode.getPositionXParam()->setValue(1.0f);

  auto buffer = std::make_shared<DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannelByType(AudioBuffer::ChannelMono))[i] = 1.0f;
  }

  panNode.setInputBuffer(buffer);
  panNode.processNode(FRAMES_TO_PROCESS);

  auto resultBuffer = panNode.getOutputBuffer();
  const float left = (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[0];
  const float right = (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[0];

  EXPECT_GT(right, left);
  EXPECT_NEAR(left + right, 1.0f, 0.05f);
}

TEST_F(PannerTest, DistanceAttenuatesSignal) {
  static constexpr int FRAMES_TO_PROCESS = 4;
  TestablePannerNode panNode(context, listener.get());
  panNode.getPositionZParam()->setValue(-10.0f);
  panNode.setRefDistance(1.0);
  panNode.setRolloffFactor(1.0);
  panNode.setDistanceModel(DistanceModelType::Inverse);

  auto buffer = std::make_shared<DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannelByType(AudioBuffer::ChannelMono))[i] = 1.0f;
  }

  panNode.setInputBuffer(buffer);
  panNode.processNode(FRAMES_TO_PROCESS);

  auto resultBuffer = panNode.getOutputBuffer();
  const float left = (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[0];
  const float right = (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[0];

  EXPECT_LT(left + right, 1.0f);
}

// NOLINTEND
