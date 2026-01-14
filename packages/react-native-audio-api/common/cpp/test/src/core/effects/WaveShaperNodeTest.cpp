#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/WaveShaperNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

class WaveShaperNodeTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * sampleRate, sampleRate, eventRegistry, RuntimeRegistry{});
  }
};

class TestableWaveShaperNode : public WaveShaperNode {
 public:
  explicit TestableWaveShaperNode(std::shared_ptr<BaseAudioContext> context)
      : WaveShaperNode(context) {
    testCurve_ = std::make_shared<AudioArray>(3);
    auto data = testCurve_->getData();
    data[0] = -2.0f;
    data[1] = 0.0f;
    data[2] = 2.0f;
  }

  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override {
    return WaveShaperNode::processNode(processingBus, framesToProcess);
  }

  std::shared_ptr<AudioArray> testCurve_;
};

TEST_F(WaveShaperNodeTest, WaveShaperNodeCanBeCreated) {
  auto waveShaper = context->createWaveShaper();
  ASSERT_NE(waveShaper, nullptr);
}

TEST_F(WaveShaperNodeTest, NullCanBeAsignedToCurve) {
  auto waveShaper = context->createWaveShaper();
  ASSERT_NO_THROW(waveShaper->setCurve(nullptr));
  ASSERT_EQ(waveShaper->getCurve(), nullptr);
}

TEST_F(WaveShaperNodeTest, NoneOverSamplingProcessesCorrectly) {
  static constexpr int FRAMES_TO_PROCESS = 5;
  auto waveShaper = std::make_shared<TestableWaveShaperNode>(context);
  waveShaper->setOversample("none");
  waveShaper->setCurve(waveShaper->testCurve_);

  auto bus = std::make_shared<audioapi::AudioBus>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < bus->getSize(); ++i) {
    bus->getChannel(0)->getData()[i] = -1.0f + i * 0.5f;
  }

  auto resultBus = waveShaper->processNode(bus, FRAMES_TO_PROCESS);
  auto curveData = waveShaper->testCurve_->getData();
  auto resultData = resultBus->getChannel(0)->getData();

  EXPECT_FLOAT_EQ(resultData[0], curveData[0]);
  EXPECT_FLOAT_EQ(resultData[1], -1.0f);
  EXPECT_FLOAT_EQ(resultData[2], 0.0f);
  EXPECT_FLOAT_EQ(resultData[3], 1.0f);
  EXPECT_FLOAT_EQ(resultData[4], curveData[2]);
}
