#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/WaveShaperNode.h>
#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArrayBuffer.hpp>
#include <audioapi/utils/AudioBuffer.h>
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
      : WaveShaperNode(context, WaveShaperOptions()) {
    testCurve_ = std::make_shared<AudioArrayBuffer>(3);
    auto data = testCurve_->span();
    data[0] = -2.0f;
    data[1] = 0.0f;
    data[2] = 2.0f;
  }

  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override {
    return WaveShaperNode::processNode(processingBuffer, framesToProcess);
  }

  std::shared_ptr<AudioArrayBuffer> testCurve_;
};

TEST_F(WaveShaperNodeTest, WaveShaperNodeCanBeCreated) {
  auto waveShaper = context->createWaveShaper(WaveShaperOptions());
  ASSERT_NE(waveShaper, nullptr);
}

TEST_F(WaveShaperNodeTest, NullCanBeAsignedToCurve) {
  auto waveShaper = context->createWaveShaper(WaveShaperOptions());
  ASSERT_NO_THROW(waveShaper->setCurve(nullptr));
  ASSERT_EQ(waveShaper->getCurve(), nullptr);
}

TEST_F(WaveShaperNodeTest, NoneOverSamplingProcessesCorrectly) {
  static constexpr int FRAMES_TO_PROCESS = 5;
  auto waveShaper = std::make_shared<TestableWaveShaperNode>(context);
  waveShaper->setOversample(OverSampleType::OVERSAMPLE_NONE);
  waveShaper->setCurve(waveShaper->testCurve_);

  auto buffer = std::make_shared<audioapi::AudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = -1.0f + i * 0.5f;
  }

  auto resultBuffer = waveShaper->processNode(buffer, FRAMES_TO_PROCESS);
  auto curveData = waveShaper->testCurve_->span();
  auto resultData = resultBuffer->getChannel(0)->span();

  EXPECT_FLOAT_EQ(resultData[0], curveData[0]);
  EXPECT_FLOAT_EQ(resultData[1], -1.0f);
  EXPECT_FLOAT_EQ(resultData[2], 0.0f);
  EXPECT_FLOAT_EQ(resultData[3], 1.0f);
  EXPECT_FLOAT_EQ(resultData[4], curveData[2]);
}
