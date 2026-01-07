#include <audioapi/core/AudioParam.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>
#include <vector>

using namespace audioapi;

class AudioParamTest : public ::testing::Test {
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

TEST_F(AudioParamTest, ValueSetters) {
  auto param = AudioParam(0.5, 0.0, 1.0, context);
  param.setValue(0.8);
  EXPECT_FLOAT_EQ(param.getValue(), 0.8);
  param.setValue(-0.5);
  EXPECT_FLOAT_EQ(param.getValue(), 0.0);
  param.setValue(1.5);
  EXPECT_FLOAT_EQ(param.getValue(), 1.0);
}

TEST_F(AudioParamTest, SetValueAtTime) {
  auto param = AudioParam(0.5, 0.0, 1.0, context);
  param.setValueAtTime(0.8, 0.1);
  param.setValueAtTime(0.3, 0.2);

  float value = param.processKRateParam(1, 0.05);
  EXPECT_FLOAT_EQ(value, 0.5);

  value = param.processKRateParam(1, 0.1);
  EXPECT_FLOAT_EQ(value, 0.8);

  value = param.processKRateParam(1, 0.15);
  EXPECT_FLOAT_EQ(value, 0.8);

  value = param.processKRateParam(1, 0.2);
  EXPECT_FLOAT_EQ(value, 0.3);

  value = param.processKRateParam(1, 0.25);
  EXPECT_FLOAT_EQ(value, 0.3);
}

TEST_F(AudioParamTest, LinearRampToValueAtTime) {
  auto param = AudioParam(0, 0, 1.0, context);
  param.linearRampToValueAtTime(1.0, 0.2);

  float value = param.processKRateParam(1, 0.05);
  EXPECT_FLOAT_EQ(value, 0.25);

  value = param.processKRateParam(1, 0.1);
  EXPECT_FLOAT_EQ(value, 0.5);

  value = param.processKRateParam(1, 0.15);
  EXPECT_FLOAT_EQ(value, 0.75);

  value = param.processKRateParam(1, 0.2);
  EXPECT_FLOAT_EQ(value, 1.0);

  value = param.processKRateParam(1, 0.25);
  EXPECT_FLOAT_EQ(value, 1.0);
}

TEST_F(AudioParamTest, ExponentialRampToValueAtTime) {
  auto param = AudioParam(0.1, 0.0, 1.0, context);
  param.exponentialRampToValueAtTime(1.0, 0.2);
  // value(time) = startValue * (endValue/startValue)^((time -
  // startTime)/(endTime - startTime)) value(time) = 0.1 * (1.0/0.1)^((time -
  // 0.0)/(0.2 - 0.0))
  float value = param.processKRateParam(1, 0.05);
  EXPECT_NEAR(value, 0.17783, 1e-5);

  value = param.processKRateParam(1, 0.1);
  EXPECT_NEAR(value, 0.316228, 1e-5);

  value = param.processKRateParam(1, 0.15);
  EXPECT_NEAR(value, 0.562341, 1e-5);

  value = param.processKRateParam(1, 0.2);
  EXPECT_FLOAT_EQ(value, 1.0);

  value = param.processKRateParam(1, 0.25);
  EXPECT_FLOAT_EQ(value, 1.0);
}

TEST_F(AudioParamTest, SetTargetAtTime) {
  auto param = AudioParam(0.0, 0.0, 1.0, context);
  param.setTargetAtTime(1.0, 0.1, 0.1);
  // value(time) = target + (startValue - target) * exp(-(time -
  // startTime)/timeConstant) value(time) = 1.0 + (0.0 - 1.0) * exp(-time/0.1)
  float value = param.processKRateParam(1, 0.05);
  EXPECT_FLOAT_EQ(value, 0.0);

  value = param.processKRateParam(1, 0.1);
  EXPECT_FLOAT_EQ(value, 0.0);

  value = param.processKRateParam(1, 0.15);
  EXPECT_NEAR(value, 0.393469, 1e-5);

  value = param.processKRateParam(1, 0.2);
  EXPECT_NEAR(value, 0.632120, 1e-5);

  value = param.processKRateParam(1, 0.25);
  EXPECT_NEAR(value, 0.776869, 1e-5);

  value = param.processKRateParam(1, 0.5);
  EXPECT_NEAR(value, 0.981684, 1e-5);
}

TEST_F(AudioParamTest, SetValueCurveAtTime) {
  auto param = AudioParam(0.0, 0.0, 1.0, context);
  param.setValue(0.5);
  auto curve = std::make_shared<std::vector<float>>(std::vector<float>{0.1, 0.4, 0.2, 0.8, 0.5});
  param.setValueCurveAtTime(curve, curve->size(), 0.1, 0.2);
  // 5 elements over 0.2s => each element is 0.04s apart

  float value = param.processKRateParam(1, 0.05);
  EXPECT_FLOAT_EQ(value, 0.0);

  value = param.processKRateParam(1, 0.1);
  EXPECT_FLOAT_EQ(value, 0.1);

  // k = 4/0.2 * (0.14 - 0.1) = 0.8 -> floor is 0
  // linear interpolation between 0 and 1 -> 0.1 + (0.4 - 0.1) * 0.8 = 0.34
  value = param.processKRateParam(1, 0.14);
  EXPECT_FLOAT_EQ(value, 0.34);

  // k = 4/0.2 * (0.18 - 0.1) = 1.6 -> floor is 1
  // linear interpolation between 1 and 2 -> 0.4 + (0.2 - 0.4) * 0.6 = 0.28
  value = param.processKRateParam(1, 0.18);
  EXPECT_FLOAT_EQ(value, 0.28);

  // k = 4/0.2 * (0.22 - 0.1) = 2.4 -> floor is 2
  // linear interpolation between 2 and 3 -> 0.2 + (0.8 - 0.2) * 0.4 = 0.44
  value = param.processKRateParam(1, 0.22);
  EXPECT_FLOAT_EQ(value, 0.44);

  // k = 4/0.2 * (0.26 - 0.1) = 3.2 -> floor is 3
  // linear interpolation between 3 and 4 -> 0.8 + (0.5 - 0.8) * 0.2 = 0.74
  value = param.processKRateParam(1, 0.26);
  EXPECT_FLOAT_EQ(value, 0.74);

  // k = 4/0.2 * (0.3 - 0.1) = 4.0 -> floor is 4
  // at or after end of curve -> last value
  value = param.processKRateParam(1, 0.35);
  EXPECT_FLOAT_EQ(value, 0.5);
}

TEST_F(AudioParamTest, CancelScheduledValues) {
  auto param = AudioParam(0.0, 0.0, 1.0, context);
  param.setValueAtTime(0.8, 0.1);
  param.setValueAtTime(0.3, 0.2);
  param.linearRampToValueAtTime(1.0, 0.4);
  param.cancelScheduledValues(0.15);

  float value = param.processKRateParam(1, 0.05);
  EXPECT_FLOAT_EQ(value, 0.0);

  value = param.processKRateParam(1, 0.1);
  EXPECT_FLOAT_EQ(value, 0.8);

  value = param.processKRateParam(1, 0.15);
  EXPECT_FLOAT_EQ(value, 0.8);

  // Events after cancel time are removed -> stays at last value
  value = param.processKRateParam(1, 0.2);
  EXPECT_FLOAT_EQ(value, 0.8);

  value = param.processKRateParam(1, 0.25);
  EXPECT_FLOAT_EQ(value, 0.8);
}

TEST_F(AudioParamTest, CancelAndHoldAtTime) {
  auto param = AudioParam(0.0, 0.0, 1.0, context);
  param.setValueAtTime(0.8, 0.1);
  param.linearRampToValueAtTime(1.0, 0.2);
  param.cancelAndHoldAtTime(0.15);

  float value = param.processKRateParam(1, 0.05);
  EXPECT_FLOAT_EQ(value, 0.0);

  value = param.processKRateParam(1, 0.1);
  EXPECT_FLOAT_EQ(value, 0.8);

  value = param.processKRateParam(1, 0.15);
  EXPECT_FLOAT_EQ(value, 0.9);

  // Events after cancel time are removed -> stays at last value
  value = param.processKRateParam(1, 0.2);
  EXPECT_FLOAT_EQ(value, 0.9);

  value = param.processKRateParam(1, 0.25);
  EXPECT_FLOAT_EQ(value, 0.9);
}
