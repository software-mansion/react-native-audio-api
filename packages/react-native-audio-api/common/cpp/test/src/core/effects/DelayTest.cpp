#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

class DelayTest : public ::testing::Test {
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

class TestableDelayNode : public DelayNode {
 public:
  explicit TestableDelayNode(std::shared_ptr<BaseAudioContext> context, const DelayOptions &options)
      : DelayNode(context, options) {}

  void setDelayTimeParam(float value) {
    getDelayTimeParam()->setValue(value);
  }

  void setInput(const std::shared_ptr<DSPAudioBuffer> &input) {
    size_t copyChannels = std::min(
        static_cast<size_t>(input->getNumberOfChannels()),
        static_cast<size_t>(audioBuffer_->getNumberOfChannels()));
    for (size_t ch = 0; ch < copyChannels; ch++) {
      audioBuffer_->getChannel(ch)->copy(*input->getChannel(ch), 0, 0, input->getSize());
    }
  }

  void processNode(int framesToProcess) override {
    DelayNode::processNode(framesToProcess);
  }
};

TEST_F(DelayTest, DelayCanBeCreated) {
  auto delay = std::make_shared<DelayNode>(context, DelayOptions());
  ASSERT_NE(delay, nullptr);
}

TEST_F(DelayTest, DelayWithZeroDelayOutputsInputSignal) {
  static constexpr float DELAY_TIME = 0.0f;
  static constexpr int FRAMES_TO_PROCESS = 4;
  auto options = DelayOptions();
  options.maxDelayTime = 1.0f;
  auto delayNode = TestableDelayNode(context, options);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
  }

  delayNode.setInput(buffer);
  delayNode.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = delayNode.getAudioBuffer();
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i], static_cast<float>(i + 1));
  }
}

TEST_F(DelayTest, DelayAppliesTimeShiftCorrectly) {
  float DELAY_TIME = (128.0 / context->getSampleRate()) * 0.5;
  static constexpr int FRAMES_TO_PROCESS = 128;
  auto options = DelayOptions();
  options.maxDelayTime = 1.0f;
  auto delayNode = TestableDelayNode(context, options);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
  }

  delayNode.setInput(buffer);
  delayNode.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = delayNode.getAudioBuffer();
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    if (i < FRAMES_TO_PROCESS / 2) { // First 64 samples should be zero due to delay
      EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i], 0.0f);
    } else {
      EXPECT_FLOAT_EQ(
          (*resultBuffer->getChannel(0))[i],
          static_cast<float>(
              i + 1 - FRAMES_TO_PROCESS / 2.0)); // Last 64 samples should be 1st part of buffer
    }
  }
}

TEST_F(DelayTest, DelayHandlesTailCorrectly) {
  float DELAY_TIME = (128.0 / context->getSampleRate()) * 0.5;
  static constexpr int FRAMES_TO_PROCESS = 128;
  auto options = DelayOptions();
  options.maxDelayTime = 1.0f;
  auto delayNode = TestableDelayNode(context, options);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
  }

  delayNode.setInput(buffer);
  delayNode.processNode(FRAMES_TO_PROCESS);
  // Second call uses the result of the first call as input (same as old behavior
  // where the same buffer object was passed to both calls)
  delayNode.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = delayNode.getAudioBuffer();
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    if (i < FRAMES_TO_PROCESS / 2) { // First 64 samples should be 2nd part of buffer
      EXPECT_FLOAT_EQ(
          (*resultBuffer->getChannel(0))[i], static_cast<float>(i + 1 + FRAMES_TO_PROCESS / 2.0));
    } else {
      EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i],
                      0.0f); // Last 64 samples should be zero
    }
  }
}
