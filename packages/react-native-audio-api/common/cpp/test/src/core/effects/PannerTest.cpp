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

TEST_F(PannerTest, PanModulatesInputMonoCorrectly) {
  static constexpr int FRAMES_TO_PROCESS = 4;
  TestablePannerNode panNode(context, listener.get());
  listener->getPositionXParam()->setValue(1.0f);
  listener->getPositionYParam()->setValue(0.0f);
  listener->getPositionZParam()->setValue(0.0f);

  panNode.getPositionXParam()->setValue(1.0f);

  auto monoInputBuffer = std::make_shared<DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < monoInputBuffer->getSize(); ++i) {
    (*monoInputBuffer->getChannelByType(AudioBuffer::ChannelMono))[i] = i + 1;
  }

  panNode.setInputBuffer(monoInputBuffer);
  panNode.processNode(FRAMES_TO_PROCESS);

  Vec3 sourcePosition = {1.0f, 0.0f, 0.0f};
  Vec3 listenerPosition = {0.0f, 0.0f, 0.0f};
  Vec3 listenerForward = {0.0f, 0.0f, -1.0f};
  Vec3 listenerUp = {0.0f, 1.0f, 0.0f};

  const auto inputLeftSpan = monoInputBuffer->getChannelByType(AudioBuffer::ChannelMono)->span();

  // float azimuth = panner::computeAzimuth(sourcePosition, listenerPosition, listenerForward, listenerUp);
  // std::cout << "Azimuth: " << azimuth << std::endl;
  // azimuth = panner::clampAzimuth(azimuth);
  // std::cout << "Clamped Azimuth: " << azimuth << std::endl;
  // azimuth = panner::wrapAzimuth(azimuth);
  // std::cout << "Wrapped Azimuth: " << azimuth << std::endl;

  // float x = (azimuth + DEG_90) / DEG_180;
  // std::cout << "X: " << x << std::endl;

  // float gainL = std::cos(x * PI / 2.0f);
  // float gainR = std::sin(x * PI / 2.0f);

  // std::cout << "Gain L: " << gainL << std::endl;
  // std::cout << "Gain R: " << gainR << std::endl;

  // For such given source and listener positions, the azimuth is 90 degrees.
  // gainL = -4.37114e-08, gainR = 1.0
  constexpr float EXPECTED_GAIN_L_AZIMUTH_90 = -4.37114e-08;
  constexpr float EXPECTED_GAIN_R_AZIMUTH_90 = 1.0;

  auto resultBuffer = panNode.getOutputBuffer();
  const auto outputLeftSpan = resultBuffer->getChannelByType(AudioBuffer::ChannelLeft)->span();
  const auto outputRightSpan = resultBuffer->getChannelByType(AudioBuffer::ChannelRight)->span();

  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelMono))[i],
        (i + 1) * EXPECTED_GAIN_L_AZIMUTH_90,
        1e-4);
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[i],
        (i + 1) * EXPECTED_GAIN_R_AZIMUTH_90,
        1e-4);
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
