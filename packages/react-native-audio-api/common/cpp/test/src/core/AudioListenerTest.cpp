#include <audioapi/core/AudioListener.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/utils/Constants.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

class AudioListenerTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * sampleRate, sampleRate, eventRegistry);
  }

  std::shared_ptr<AudioListener> makeListener() {
    return std::make_shared<AudioListener>(context);
  }
};

// Spec: position (0, 0, 0), forward (0, 0, -1), up (0, 1, 0).
TEST_F(AudioListenerTest, DefaultParamValues) {
  auto listener = makeListener();

  EXPECT_FLOAT_EQ(listener->getPositionXParam()->getValue(), 0.0f);
  EXPECT_FLOAT_EQ(listener->getPositionYParam()->getValue(), 0.0f);
  EXPECT_FLOAT_EQ(listener->getPositionZParam()->getValue(), 0.0f);
  EXPECT_FLOAT_EQ(listener->getForwardXParam()->getValue(), 0.0f);
  EXPECT_FLOAT_EQ(listener->getForwardYParam()->getValue(), 0.0f);
  EXPECT_FLOAT_EQ(listener->getForwardZParam()->getValue(), -1.0f);
  EXPECT_FLOAT_EQ(listener->getUpXParam()->getValue(), 0.0f);
  EXPECT_FLOAT_EQ(listener->getUpYParam()->getValue(), 1.0f);
  EXPECT_FLOAT_EQ(listener->getUpZParam()->getValue(), 0.0f);
}

TEST_F(AudioListenerTest, ParamRanges) {
  auto listener = makeListener();

  for (const auto &param :
       {listener->getPositionXParam(),
        listener->getPositionYParam(),
        listener->getPositionZParam(),
        listener->getForwardXParam(),
        listener->getForwardYParam(),
        listener->getForwardZParam(),
        listener->getUpXParam(),
        listener->getUpYParam(),
        listener->getUpZParam()}) {
    EXPECT_FLOAT_EQ(param->getMinValue(), MOST_NEGATIVE_SINGLE_FLOAT);
    EXPECT_FLOAT_EQ(param->getMaxValue(), MOST_POSITIVE_SINGLE_FLOAT);
  }
}

TEST_F(AudioListenerTest, ParamsAreSettable) {
  auto listener = makeListener();

  listener->getPositionXParam()->setValue(3.5f);
  listener->getForwardZParam()->setValue(0.25f);
  listener->getUpYParam()->setValue(-2.0f);

  EXPECT_FLOAT_EQ(listener->getPositionXParam()->getValue(), 3.5f);
  EXPECT_FLOAT_EQ(listener->getForwardZParam()->getValue(), 0.25f);
  EXPECT_FLOAT_EQ(listener->getUpYParam()->getValue(), -2.0f);
}

// NOLINTEND
