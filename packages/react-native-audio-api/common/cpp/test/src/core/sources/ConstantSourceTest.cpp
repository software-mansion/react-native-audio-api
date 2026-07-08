#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

class ConstantSourceTest : public ::testing::Test {
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

class TestableConstantSourceNode : public ConstantSourceNode {
 public:
  explicit TestableConstantSourceNode(std::shared_ptr<BaseAudioContext> context)
      : ConstantSourceNode(context, ConstantSourceOptions()) {}

  void setOffsetParam(float value) {
    getOffsetParam()->setValue(value);
  }

  void processNode(int framesToProcess) override {
    ConstantSourceNode::processNode(framesToProcess);
  }
};

TEST_F(ConstantSourceTest, ConstantSourceCanBeCreated) {
  auto constantSource = std::make_shared<ConstantSourceNode>(context, ConstantSourceOptions());
  ASSERT_NE(constantSource, nullptr);
}

TEST_F(ConstantSourceTest, ConstantSourceOutputsConstantValue) {
  static constexpr int FRAMES_TO_PROCESS = 4;

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  auto constantSource = TestableConstantSourceNode(context);
  constantSource.start(context->getCurrentTime());
  constantSource.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = constantSource.getOutputBuffer();

  for (int i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i], 1.0f);
  }

  constantSource.setOffsetParam(0.5f);
  constantSource.processNode(FRAMES_TO_PROCESS);
  resultBuffer = constantSource.getOutputBuffer();
  for (int i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i], 0.5f);
  }
}

// NOLINTEND
