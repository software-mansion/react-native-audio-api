#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/core/effects/delay/DelayReader.h>
#include <audioapi/core/effects/delay/DelayWriter.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

class DelayTest : public ::testing::Test {
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

class TestableDelayWriter : public DelayWriter {
 public:
  explicit TestableDelayWriter(
      std::shared_ptr<BaseAudioContext> context,
      const DelayOptions &options,
      std::shared_ptr<DelayLine> delayLine)
      : DelayWriter(context, options, delayLine) {}

  void setInputBuffer(const std::shared_ptr<DSPAudioBuffer> &input) {
    audioBuffer_ = input;
  }
};

class TestableDelayReader : public DelayReader {
 public:
  explicit TestableDelayReader(
      std::shared_ptr<BaseAudioContext> context,
      const DelayOptions &options,
      std::shared_ptr<DelayLine> delayLine)
      : DelayReader(context, options, delayLine) {}

  auto getOutputBuffer() {
    return audioBuffer_;
  }
};

class TestableDelayNode : public DelayNode {
 public:
  explicit TestableDelayNode(std::shared_ptr<BaseAudioContext> context, const DelayOptions &options)
      : DelayNode(context, options),
        testableDelayReader_(context, options, delayLine_),
        testableDelayWriter_(context, options, delayLine_) {}

  void setDelayTimeParam(float value) {
    getDelayTimeParam()->setValue(value);
  }

  void setInputBuffer(const std::shared_ptr<DSPAudioBuffer> &input) {
    testableDelayWriter_.setInputBuffer(input);
  }

  auto getOutputBuffer() {
    return testableDelayReader_.getOutputBuffer();
  }

  void processNode(int framesToProcess) override {
    testableDelayWriter_.processNode(framesToProcess);
    testableDelayReader_.processNode(framesToProcess);
  }

 private:
  TestableDelayWriter testableDelayWriter_;
  TestableDelayReader testableDelayReader_;
};

TEST_F(DelayTest, DelayCanBeCreated) {
  auto delay = std::make_shared<DelayNode>(context, DelayOptions());
  ASSERT_NE(delay, nullptr);
}

TEST_F(DelayTest, DelayWithZeroDelayOutputsInputSignal) {
  static constexpr float DELAY_TIME = 0.0f;
  static constexpr int FRAMES_TO_PROCESS = 128;
  auto options = DelayOptions();
  options.maxDelayTime = 1.0f;
  auto delayNode = TestableDelayNode(context, options);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
  }

  delayNode.setInputBuffer(buffer);
  delayNode.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = delayNode.getOutputBuffer();
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i], static_cast<float>(i + 1));
  }
}

TEST_F(DelayTest, DelayAppliesTimeShiftCorrectly) {
  static constexpr int FRAMES_TO_PROCESS = 128;
  float DELAY_TIME = (FRAMES_TO_PROCESS / context->getSampleRate()) * 0.5;
  auto options = DelayOptions();
  options.maxDelayTime = 1.0f;
  auto delayNode = TestableDelayNode(context, options);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
  }

  delayNode.setInputBuffer(buffer);
  delayNode.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = delayNode.getOutputBuffer();
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
  static constexpr int FRAMES_TO_PROCESS = 128;
  float DELAY_TIME = (FRAMES_TO_PROCESS / context->getSampleRate()) * 0.5;
  auto options = DelayOptions();
  options.maxDelayTime = (FRAMES_TO_PROCESS / context->getSampleRate()) * 3;
  auto delayNode = TestableDelayNode(context, options);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
  }

  delayNode.setInputBuffer(buffer);
  delayNode.processNode(FRAMES_TO_PROCESS);
  // Second call uses the result of the first call as input
  delayNode.getOutputBuffer()->zero();
  delayNode.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = delayNode.getOutputBuffer();
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    if (i < FRAMES_TO_PROCESS / 2) { // First samples should be 2nd part of buffer
      EXPECT_FLOAT_EQ(
          (*resultBuffer->getChannel(0))[i], static_cast<float>(i + 1 + FRAMES_TO_PROCESS / 2.0));
    } else {
      EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i],
                      0.0f); // Last samples should be zero
    }
  }
}

// NOLINTEND
