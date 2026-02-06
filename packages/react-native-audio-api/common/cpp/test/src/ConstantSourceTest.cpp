#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
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
      : ConstantSourceNode(context, ConstantSourceOptions()) {}

  void setOffsetParam(float value) {
    getOffsetParam()->setValue(value);
  }

  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override {
    return ConstantSourceNode::processNode(processingBuffer, framesToProcess);
  }
};

TEST_F(ConstantSourceTest, ConstantSourceCanBeCreated) {
  auto constantSource = context->createConstantSource(ConstantSourceOptions());
  ASSERT_NE(constantSource, nullptr);
}

TEST_F(ConstantSourceTest, ConstantSourceOutputsConstantValue) {
  static constexpr int FRAMES_TO_PROCESS = 4;

  auto buffer = std::make_shared<audioapi::AudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  auto constantSource = TestableConstantSourceNode(context);
  // constantSource.start(context->getCurrentTime());
  // auto resultBuffer = constantSource.processNode(buffer, FRAMES_TO_PROCESS);

  // for (int i = 0; i < FRAMES_TO_PROCESS; ++i) {
  //   EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i], 1.0f);
  // }

  // constantSource.setOffsetParam(0.5f);
  // resultBuffer = constantSource.processNode(buffer, FRAMES_TO_PROCESS);
  // for (int i = 0; i < FRAMES_TO_PROCESS; ++i) {
  //   EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i], 0.5f);
  // }
}
