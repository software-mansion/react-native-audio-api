#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
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
  explicit TestableDelayNode(std::shared_ptr<BaseAudioContext> context) : DelayNode(context, 1) {}

  void setDelayTimeParam(float value) {
    getDelayTimeParam()->setValue(value);
  }

  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override {
    return DelayNode::processNode(processingBus, framesToProcess);
  }
};

TEST_F(DelayTest, DelayCanBeCreated) {
  auto delay = context->createDelay(1.0f);
  ASSERT_NE(delay, nullptr);
}

TEST_F(DelayTest, DelayWithZeroDelayOutputsInputSignal) {
  static constexpr float DELAY_TIME = 0.0f;
  static constexpr int FRAMES_TO_PROCESS = 4;
  auto delayNode = TestableDelayNode(context);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto bus = std::make_shared<audioapi::AudioBus>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < bus->getSize(); ++i) {
    bus->getChannel(0)->getData()[i] = i + 1;
  }

  auto resultBus = delayNode.processNode(bus, FRAMES_TO_PROCESS);
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_FLOAT_EQ((*resultBus->getChannel(0))[i], static_cast<float>(i + 1));
  }
}

TEST_F(DelayTest, DelayAppliesTimeShiftCorrectly) {
  float DELAY_TIME = (128.0 / context->getSampleRate()) * 0.5;
  static constexpr int FRAMES_TO_PROCESS = 128;
  auto delayNode = TestableDelayNode(context);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto bus = std::make_shared<audioapi::AudioBus>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < bus->getSize(); ++i) {
    bus->getChannel(0)->getData()[i] = i + 1;
  }

  auto resultBus = delayNode.processNode(bus, FRAMES_TO_PROCESS);
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    if (i < FRAMES_TO_PROCESS / 2) { // First 64 samples should be zero due to delay
      EXPECT_FLOAT_EQ((*resultBus->getChannel(0))[i], 0.0f);
    } else {
      EXPECT_FLOAT_EQ(
          (*resultBus->getChannel(0))[i],
          static_cast<float>(
              i + 1 - FRAMES_TO_PROCESS / 2)); // Last 64 samples should be 1st part of bus
    }
  }
}

TEST_F(DelayTest, DelayHandlesTailCorrectly) {
  float DELAY_TIME = (128.0 / context->getSampleRate()) * 0.5;
  static constexpr int FRAMES_TO_PROCESS = 128;
  auto delayNode = TestableDelayNode(context);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto bus = std::make_shared<audioapi::AudioBus>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < bus->getSize(); ++i) {
    bus->getChannel(0)->getData()[i] = i + 1;
  }

  delayNode.processNode(bus, FRAMES_TO_PROCESS);
  auto resultBus = delayNode.processNode(bus, FRAMES_TO_PROCESS);
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    if (i < FRAMES_TO_PROCESS / 2) { // First 64 samples should be 2nd part of bus
      EXPECT_FLOAT_EQ(
          (*resultBus->getChannel(0))[i], static_cast<float>(i + 1 + FRAMES_TO_PROCESS / 2));
    } else {
      EXPECT_FLOAT_EQ((*resultBus->getChannel(0))[i],
                      0.0f); // Last 64 samples should be zero
    }
  }
}
