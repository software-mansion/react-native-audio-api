#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/GainNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

class GainTest : public ::testing::Test {
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

class TestableGainNode : public GainNode {
 public:
  explicit TestableGainNode(std::shared_ptr<BaseAudioContext> context) : GainNode(context) {}

  void setGainParam(float value) {
    getGainParam()->setValue(value);
  }

  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override {
    return GainNode::processNode(processingBus, framesToProcess);
  }
};

TEST_F(GainTest, GainCanBeCreated) {
  auto gain = context->createGain();
  ASSERT_NE(gain, nullptr);
}

TEST_F(GainTest, GainModulatesVolumeCorrectly) {
  static constexpr float GAIN_VALUE = 0.5f;
  static constexpr int FRAMES_TO_PROCESS = 4;
  auto gainNode = TestableGainNode(context);
  gainNode.setGainParam(GAIN_VALUE);

  auto bus = std::make_shared<audioapi::AudioBus>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < bus->getSize(); ++i) {
    bus->getChannel(0)->getData()[i] = i + 1;
  }

  auto resultBus = gainNode.processNode(bus, FRAMES_TO_PROCESS);
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_FLOAT_EQ((*resultBus->getChannel(0))[i], (i + 1) * GAIN_VALUE);
  }
}

TEST_F(GainTest, GainModulatesVolumeCorrectlyMultiChannel) {
  static constexpr float GAIN_VALUE = 0.5f;
  static constexpr int FRAMES_TO_PROCESS = 4;
  auto gainNode = TestableGainNode(context);
  gainNode.setGainParam(GAIN_VALUE);

  auto bus = std::make_shared<audioapi::AudioBus>(FRAMES_TO_PROCESS, 2, sampleRate);
  for (size_t i = 0; i < bus->getSize(); ++i) {
    bus->getChannel(0)->getData()[i] = i + 1;
    bus->getChannel(1)->getData()[i] = -i - 1;
  }

  auto resultBus = gainNode.processNode(bus, FRAMES_TO_PROCESS);
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_FLOAT_EQ((*resultBus->getChannel(0))[i], (i + 1) * GAIN_VALUE);
    EXPECT_FLOAT_EQ((*resultBus->getChannel(1))[i], (-i - 1) * GAIN_VALUE);
  }
}
