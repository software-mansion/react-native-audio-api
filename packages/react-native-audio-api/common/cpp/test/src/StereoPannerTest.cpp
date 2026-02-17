#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/StereoPannerNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

class StereoPannerTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * sampleRate, sampleRate, eventRegistry, RuntimeRegistry{});
    context->initialize();
  }
};

class TestableStereoPannerNode : public StereoPannerNode {
 public:
  explicit TestableStereoPannerNode(std::shared_ptr<BaseAudioContext> context)
      : StereoPannerNode(context, StereoPannerOptions()) {}

  void setPanParam(float value) {
    getPanParam()->setValue(value);
  }

  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override {
    return StereoPannerNode::processNode(processingBuffer, framesToProcess);
  }
};

TEST_F(StereoPannerTest, StereoPannerCanBeCreated) {
  auto panner = context->createStereoPanner(StereoPannerOptions());
  ASSERT_NE(panner, nullptr);
}

TEST_F(StereoPannerTest, PanModulatesInputMonoCorrectly) {
  static constexpr float PAN_VALUE = 0.5;
  static constexpr int FRAMES_TO_PROCESS = 4;
  auto panNode = TestableStereoPannerNode(context);
  panNode.setPanParam(PAN_VALUE);

  auto buffer = std::make_shared<audioapi::AudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannelByType(AudioBuffer::ChannelLeft))[i] = i + 1;
  }

  auto resultBuffer = panNode.processNode(buffer, FRAMES_TO_PROCESS);
  // x = (0.5 + 1) / 2 = 0.75
  // gainL = cos(x * (π / 2)) = cos(0.75 * (π / 2)) = 0.38268343236508984
  // gainR = sin(x * (π / 2)) = sin(0.75 * (π / 2)) = 0.9238795325112867
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i],
        (i + 1) * 0.38268343236508984,
        1e-4);
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[i],
        (i + 1) * 0.9238795325112867,
        1e-4);
  }
}

TEST_F(StereoPannerTest, PanModulatesInputStereoCorrectlyWithNegativePan) {
  static constexpr float PAN_VALUE = -0.5;
  static constexpr int FRAMES_TO_PROCESS = 4;
  auto panNode = TestableStereoPannerNode(context);
  panNode.setPanParam(PAN_VALUE);

  auto buffer = std::make_shared<audioapi::AudioBuffer>(FRAMES_TO_PROCESS, 2, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannelByType(AudioBuffer::ChannelLeft))[i] = i + 1;
    (*buffer->getChannelByType(AudioBuffer::ChannelRight))[i] = i + 1;
  }

  auto resultBuffer = panNode.processNode(buffer, FRAMES_TO_PROCESS);
  // x = -0.5 + 1 = 0.5
  // gainL = cos(x * (π / 2)) = cos(0.5 * (π / 2)) = 0.7071067811865476
  // gainR = sin(x * (π / 2)) = sin(0.5 * (π / 2)) = 0.7071067811865476
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i],
        (i + 1) + (i + 1) * 0.7071067811865476,
        1e-4);
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[i],
        (i + 1) * 0.7071067811865476,
        1e-4);
  }
}

TEST_F(StereoPannerTest, PanModulatesInputStereoCorrectlyWithPositivePan) {
  static constexpr float PAN_VALUE = 0.75;
  static constexpr int FRAMES_TO_PROCESS = 4;
  auto panNode = TestableStereoPannerNode(context);
  panNode.setPanParam(PAN_VALUE);

  auto buffer = std::make_shared<audioapi::AudioBuffer>(FRAMES_TO_PROCESS, 2, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannelByType(AudioBuffer::ChannelLeft))[i] = i + 1;
    (*buffer->getChannelByType(AudioBuffer::ChannelRight))[i] = i + 1;
  }

  auto resultBuffer = panNode.processNode(buffer, FRAMES_TO_PROCESS);
  // x = 0.75
  // gainL = cos(x * (π / 2)) = cos(0.75 * (π / 2)) = 0.38268343236508984
  // gainR = sin(x * (π / 2)) = sin(0.75 * (π / 2)) = 0.9238795325112867
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[i],
        (i + 1) * 0.38268343236508984,
        1e-4);
    EXPECT_NEAR(
        (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[i],
        (i + 1) + (i + 1) * 0.9238795325112867,
        1e-4);
  }
}
