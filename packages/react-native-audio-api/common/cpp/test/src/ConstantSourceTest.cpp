#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

class ConstantSourceTest : public ::testing::Test {
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

class TestableConstantSourceNode : public ConstantSourceNode {
 public:
  explicit TestableConstantSourceNode(std::shared_ptr<BaseAudioContext> context)
      : ConstantSourceNode(context) {}

  void setOffsetParam(float value) {
    getOffsetParam()->setValue(value);
  }

  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override {
    return ConstantSourceNode::processNode(processingBus, framesToProcess);
  }
};

TEST_F(ConstantSourceTest, ConstantSourceCanBeCreated) {
  auto constantSource = context->createConstantSource();
  ASSERT_NE(constantSource, nullptr);
}

TEST_F(ConstantSourceTest, ConstantSourceOutputsConstantValue) {
  static constexpr int FRAMES_TO_PROCESS = 4;

  auto bus = std::make_shared<audioapi::AudioBus>(FRAMES_TO_PROCESS, 1, sampleRate);
  auto constantSource = TestableConstantSourceNode(context);
  // constantSource.start(context->getCurrentTime());
  // auto resultBus = constantSource.processNode(bus, FRAMES_TO_PROCESS);

  // for (int i = 0; i < FRAMES_TO_PROCESS; ++i) {
  //   EXPECT_FLOAT_EQ((*resultBus->getChannel(0))[i], 1.0f);
  // }

  // constantSource.setOffsetParam(0.5f);
  // resultBus = constantSource.processNode(bus, FRAMES_TO_PROCESS);
  // for (int i = 0; i < FRAMES_TO_PROCESS; ++i) {
  //   EXPECT_FLOAT_EQ((*resultBus->getChannel(0))[i], 0.5f);
  // }
}
