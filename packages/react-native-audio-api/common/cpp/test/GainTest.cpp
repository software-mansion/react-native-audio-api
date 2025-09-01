#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/GainNode.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <gtest/gtest.h>
#include "MockAudioEventHandlerRegistry.h"

class GainTest : public ::testing::Test {
 protected:
  std::shared_ptr<audioapi::IAudioEventHandlerRegistry> eventRegistry;
  std::unique_ptr<audioapi::OfflineAudioContext> context;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_unique<audioapi::OfflineAudioContext>(
        2, 5 * sampleRate, sampleRate, eventRegistry, nullptr);
  }
};

class TestableGainNode : public audioapi::GainNode {
 public:
  explicit TestableGainNode(audioapi::BaseAudioContext *context)
      : audioapi::GainNode(context) {}

  void setGainParam(float value) {
    getGainParam()->setValue(value);
  }

  void processNode(
      const std::shared_ptr<audioapi::AudioBus> &processingBus,
      int framesToProcess) override {
    audioapi::GainNode::processNode(processingBus, framesToProcess);
  }
};

TEST_F(GainTest, GainCanBeCreated) {
  auto gain = context->createGain();
  ASSERT_NE(gain, nullptr);
}

TEST_F(GainTest, GainModulatesVolumeCorrectly) {
  static constexpr float GAIN_VALUE = 0.5f;
  static constexpr int FRAMES_TO_PROCESS = 4;
  auto gainNode = std::make_shared<TestableGainNode>(context.get());
  gainNode->setGainParam(GAIN_VALUE);

  auto bus =
      std::make_shared<audioapi::AudioBus>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < bus->getSize(); ++i) {
    bus->getChannel(0)->getData()[i] = i + 1;
  }

  gainNode->processNode(bus, FRAMES_TO_PROCESS);
  for (size_t i = 0; i < bus->getSize(); ++i) {
    EXPECT_FLOAT_EQ((*bus->getChannel(0))[i], (i + 1) * GAIN_VALUE);
  }
}

TEST_F(GainTest, GainModulatesVolumeCorrectlyMultiChannel) {
  static constexpr float GAIN_VALUE = 0.5f;
  static constexpr int FRAMES_TO_PROCESS = 4;
  auto gainNode = std::make_shared<TestableGainNode>(context.get());
  gainNode->setGainParam(GAIN_VALUE);

  auto bus =
      std::make_shared<audioapi::AudioBus>(FRAMES_TO_PROCESS, 2, sampleRate);
  for (size_t i = 0; i < bus->getSize(); ++i) {
    bus->getChannel(0)->getData()[i] = i + 1;
    bus->getChannel(1)->getData()[i] = -i - 1;
  }

  gainNode->processNode(bus, FRAMES_TO_PROCESS);
  for (size_t i = 0; i < bus->getSize(); ++i) {
    EXPECT_FLOAT_EQ((*bus->getChannel(0))[i], (i + 1) * GAIN_VALUE);
    EXPECT_FLOAT_EQ((*bus->getChannel(1))[i], (-i - 1) * GAIN_VALUE);
  }
}
