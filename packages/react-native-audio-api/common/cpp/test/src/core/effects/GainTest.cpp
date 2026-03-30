#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/GainNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

class GainTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * sampleRate, sampleRate, eventRegistry, RuntimeRegistry{});
    destination = std::make_shared<AudioDestinationNode>(context);
    context->initialize(destination.get());
  }
};

class TestableGainNode : public GainNode {
 public:
  explicit TestableGainNode(std::shared_ptr<BaseAudioContext> context)
      : GainNode(context, GainOptions()) {}

  void setGainParam(float value) {
    getGainParam()->setValue(value);
  }

  void setInputBuffer(const std::shared_ptr<DSPAudioBuffer> &input) {
    audioBuffer_ = input;
  }

  void processNode(int framesToProcess) override {
    GainNode::processNode(framesToProcess);
  }
};

TEST_F(GainTest, GainCanBeCreated) {
  auto gain = std::make_shared<GainNode>(context, GainOptions());
  ASSERT_NE(gain, nullptr);
}

TEST_F(GainTest, GainModulatesVolumeCorrectly) {
  static constexpr float GAIN_VALUE = 0.5f;
  static constexpr int FRAMES_TO_PROCESS = 4;
  auto gainNode = TestableGainNode(context);
  gainNode.setGainParam(GAIN_VALUE);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
  }

  gainNode.setInputBuffer(buffer);
  gainNode.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = gainNode.getOutputBuffer();
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i], (i + 1) * GAIN_VALUE);
  }
}

TEST_F(GainTest, GainModulatesVolumeCorrectlyMultiChannel) {
  static constexpr float GAIN_VALUE = 0.5f;
  static constexpr int FRAMES_TO_PROCESS = 4;
  auto gainNode = TestableGainNode(context);
  gainNode.setGainParam(GAIN_VALUE);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 2, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
    (*buffer->getChannel(1))[i] = -i - 1;
  }

  gainNode.setInputBuffer(buffer);
  gainNode.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = gainNode.getOutputBuffer();
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i], (i + 1) * GAIN_VALUE);
    EXPECT_FLOAT_EQ((*resultBuffer->getChannel(1))[i], (-i - 1) * GAIN_VALUE);
  }
}

// NOLINTEND
