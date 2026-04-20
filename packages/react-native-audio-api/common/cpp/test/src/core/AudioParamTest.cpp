#include <audioapi/core/AudioParam.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

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

class TestableAudioParam : public AudioParam {
 public:
  explicit TestableAudioParam(
      float defaultValue,
      float minValue,
      float maxValue,
      std::shared_ptr<BaseAudioContext> context)
      : AudioParam(defaultValue, minValue, maxValue, context) {}

  float process(double time) {
    return AudioParam::processKRateParam(1, time);
  }
};

// --- Basic ---

TEST_F(AudioParamTest, ValueSetters) {
  auto param = TestableAudioParam(0.5, 0.0, 1.0, context);
  param.setValue(0.8);
  EXPECT_FLOAT_EQ(param.getValue(), 0.8);
  param.setValue(-0.5);
  EXPECT_FLOAT_EQ(param.getValue(), 0.0);
  param.setValue(1.5);
  EXPECT_FLOAT_EQ(param.getValue(), 1.0);
}

// No events scheduled — falls back to value_.
TEST_F(AudioParamTest, NoEventsReturnsCurrentValue) {
  auto param = TestableAudioParam(0.5, 0.0, 1.0, context);
  EXPECT_FLOAT_EQ(param.process(0.0), 0.5f);
  EXPECT_FLOAT_EQ(param.process(1.0), 0.5f);

  param.setValue(0.7f);
  EXPECT_FLOAT_EQ(param.process(0.0), 0.7f);
}

// --- Event types ---

TEST_F(AudioParamTest, SetValueAtTime) {
  auto param = TestableAudioParam(0.5, 0.0, 1.0, context);
  param.setValueAtTime(0.8, 0.1);
  param.setValueAtTime(0.3, 0.2);

  EXPECT_FLOAT_EQ(param.process(0.05), 0.5f);
  EXPECT_FLOAT_EQ(param.process(0.1), 0.8f);
  EXPECT_FLOAT_EQ(param.process(0.15), 0.8f);
  EXPECT_FLOAT_EQ(param.process(0.2), 0.3f);
  EXPECT_FLOAT_EQ(param.process(0.25), 0.3f);
}

TEST_F(AudioParamTest, LinearRampToValueAtTime) {
  auto param = TestableAudioParam(0, 0, 1.0, context);
  param.linearRampToValueAtTime(1.0, 0.2);

  EXPECT_FLOAT_EQ(param.process(0.05), 0.25f);
  EXPECT_FLOAT_EQ(param.process(0.1), 0.5f);
  EXPECT_FLOAT_EQ(param.process(0.15), 0.75f);
  EXPECT_FLOAT_EQ(param.process(0.2), 1.0f);
  EXPECT_FLOAT_EQ(param.process(0.25), 1.0f);
}

TEST_F(AudioParamTest, ExponentialRampToValueAtTime) {
  auto param = TestableAudioParam(0.1, 0.0, 1.0, context);
  param.exponentialRampToValueAtTime(1.0, 0.2);
  // value(time) = startValue * (endValue/startValue)^((time - startTime)/(endTime - startTime))
  // value(time) = 0.1 * (1.0/0.1)^((time - 0.0)/(0.2 - 0.0))

  EXPECT_NEAR(param.process(0.05), 0.17783f, 1e-5f);
  EXPECT_NEAR(param.process(0.1), 0.316228f, 1e-5f);
  EXPECT_NEAR(param.process(0.15), 0.562341f, 1e-5f);
  EXPECT_FLOAT_EQ(param.process(0.2), 1.0f);
  EXPECT_FLOAT_EQ(param.process(0.25), 1.0f);
}

TEST_F(AudioParamTest, SetTargetAtTime) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.setTargetAtTime(1.0, 0.1, 0.1);
  // value(time) = target + (startValue - target) * exp(-(time - startTime)/timeConstant)
  // value(time) = 1.0 + (0.0 - 1.0) * exp(-time/0.1)

  EXPECT_FLOAT_EQ(param.process(0.05), 0.0f);
  EXPECT_FLOAT_EQ(param.process(0.1), 0.0f);
  EXPECT_NEAR(param.process(0.15), 0.393469f, 1e-5f);
  EXPECT_NEAR(param.process(0.2), 0.632120f, 1e-5f);
  EXPECT_NEAR(param.process(0.25), 0.776869f, 1e-5f);
  EXPECT_NEAR(param.process(0.5), 0.981684f, 1e-5f);
}

TEST_F(AudioParamTest, SetValueCurveAtTime) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.setValue(0.5);
  auto curve = std::make_shared<AudioArray>(5);
  auto curveSpan = curve->span();
  curveSpan[0] = 0.1f;
  curveSpan[1] = 0.4f;
  curveSpan[2] = 0.2f;
  curveSpan[3] = 0.8f;
  curveSpan[4] = 0.5f;
  param.setValueCurveAtTime(curve, curve->getSize(), 0.1, 0.2);
  // 5 elements over 0.2s => each element is 0.04s apart

  EXPECT_FLOAT_EQ(param.process(0.05), 0.0f);
  EXPECT_FLOAT_EQ(param.process(0.1), 0.1f);
  // k = 4/0.2 * (0.14 - 0.1) = 0.8 -> floor is 0
  // linear interpolation between 0 and 1 -> 0.1 + (0.4 - 0.1) * 0.8 = 0.34
  EXPECT_FLOAT_EQ(param.process(0.14), 0.34f);
  // k = 4/0.2 * (0.18 - 0.1) = 1.6 -> floor is 1
  // linear interpolation between 1 and 2 -> 0.4 + (0.2 - 0.4) * 0.6 = 0.28
  EXPECT_FLOAT_EQ(param.process(0.18), 0.28f);
  // k = 4/0.2 * (0.22 - 0.1) = 2.4 -> floor is 2
  // linear interpolation between 2 and 3 -> 0.2 + (0.8 - 0.2) * 0.4 = 0.44
  EXPECT_FLOAT_EQ(param.process(0.22), 0.44f);
  // k = 4/0.2 * (0.26 - 0.1) = 3.2 -> floor is 3
  // linear interpolation between 3 and 4 -> 0.8 + (0.5 - 0.8) * 0.2 = 0.74
  EXPECT_FLOAT_EQ(param.process(0.26), 0.74f);
  // k = 4/0.2 * (0.3 - 0.1) = 4.0 -> floor is 4
  // at or after end of curve -> last value
  EXPECT_FLOAT_EQ(param.process(0.35), 0.5f);
}

// --- Insertion between elements ---

// Insert a linear ramp between two setValueAtTime events.
TEST_F(AudioParamTest, LinearRampInsertedBetweenSetValueAtTimeEvents) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.setValueAtTime(0.2, 0.1);
  param.setValueAtTime(0.8, 0.3);
  // Insert ramp between them — ramps from 0.2 to 0.6 over [0.1, 0.2]
  param.linearRampToValueAtTime(0.6, 0.2);

  EXPECT_FLOAT_EQ(param.process(0.05), 0.0f);
  EXPECT_FLOAT_EQ(param.process(0.1), 0.2f);
  // midpoint: 0.2 + (0.6 - 0.2) * 0.5
  EXPECT_FLOAT_EQ(param.process(0.15), 0.4f);
  EXPECT_FLOAT_EQ(param.process(0.2), 0.6f);
  EXPECT_FLOAT_EQ(param.process(0.25), 0.6f);
  EXPECT_FLOAT_EQ(param.process(0.3), 0.8f);
}

// Insert an exponential ramp between two setValueAtTime events.
TEST_F(AudioParamTest, ExponentialRampInsertedBetweenSetValueAtTimeEvents) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.setValueAtTime(0.1, 0.0);
  param.setValueAtTime(0.8, 0.4);
  // Insert exponential ramp: value(t) = 0.1 * 10^(t/0.2)
  param.exponentialRampToValueAtTime(1.0, 0.2);

  EXPECT_FLOAT_EQ(param.process(0.0), 0.1f);
  EXPECT_NEAR(param.process(0.1), 0.1f * std::pow(10.0f, 0.5f), 1e-5f);
  EXPECT_FLOAT_EQ(param.process(0.2), 1.0f);
  EXPECT_FLOAT_EQ(param.process(0.3), 1.0f);
  EXPECT_FLOAT_EQ(param.process(0.4), 0.8f);
}

// Insert a setValueAtTime before an already-scheduled ramp;
// the ramp's startTime and startValue update to chain from the new event.
TEST_F(AudioParamTest, SetValueAtTimeInsertedBeforeRampUpdatesRampChain) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.linearRampToValueAtTime(1.0, 0.4);
  // ramp should now start from 0.5 at t=0.2
  param.setValueAtTime(0.5, 0.2);

  EXPECT_FLOAT_EQ(param.process(0.1), 0.0f);
  EXPECT_FLOAT_EQ(param.process(0.2), 0.5f);
  // midpoint: 0.5 + (1.0 - 0.5) * 0.5
  EXPECT_FLOAT_EQ(param.process(0.3), 0.75f);
  EXPECT_FLOAT_EQ(param.process(0.4), 1.0f);
  EXPECT_FLOAT_EQ(param.process(0.5), 1.0f);
}

// Insert a setValueAtTime between two ramps: second ramp's startTime/startValue update.
TEST_F(AudioParamTest, SetValueAtTimeInsertedBetweenTwoRamps) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.linearRampToValueAtTime(0.5, 0.2); // ramp1: 0->0.5 over [0, 0.2]
  param.linearRampToValueAtTime(1.0, 0.4); // ramp2: 0.5->1.0 over [0.2, 0.4]
  // Insert setValue at t=0.2 — ramp2 now starts at 0.3
  param.setValueAtTime(0.3, 0.2);

  // midpoint of ramp1: 0 + 0.5 * 0.5
  EXPECT_FLOAT_EQ(param.process(0.1), 0.25f);
  EXPECT_FLOAT_EQ(param.process(0.2), 0.3f);
  // midpoint of ramp2: 0.3 + (1.0 - 0.3) * 0.5
  EXPECT_NEAR(param.process(0.3), 0.65f, 1e-5f);
  EXPECT_FLOAT_EQ(param.process(0.4), 1.0f);
}

// Insert a setValueAtTime after a setTarget: resolveEventValues caps setTarget's endTime.
TEST_F(AudioParamTest, SetValueAtTimeInsertedAfterSetTargetCapsIt) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  // value(t) = 1.0 - exp(-t/0.1)
  param.setTargetAtTime(1.0, 0.0, 0.1);
  param.setValueAtTime(0.5, 0.2);

  EXPECT_NEAR(param.process(0.1), 1.0f - std::exp(-1.0f), 1e-5f);
  EXPECT_FLOAT_EQ(param.process(0.2), 0.5f);
  EXPECT_FLOAT_EQ(param.process(0.3), 0.5f);
}

// --- cancelScheduledValues ---

TEST_F(AudioParamTest, CancelScheduledValues) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.setValueAtTime(0.8, 0.1);
  param.setValueAtTime(0.3, 0.2);
  param.linearRampToValueAtTime(1.0, 0.4);
  param.cancelScheduledValues(0.15);

  EXPECT_FLOAT_EQ(param.process(0.05), 0.0f);
  EXPECT_FLOAT_EQ(param.process(0.1), 0.8f);
  EXPECT_FLOAT_EQ(param.process(0.15), 0.8f);
  // Events after cancel time are removed -> stays at last value
  EXPECT_FLOAT_EQ(param.process(0.2), 0.8f);
  EXPECT_FLOAT_EQ(param.process(0.25), 0.8f);
}

// Cancel before all events empties the queue; falls back to value_.
TEST_F(AudioParamTest, CancelScheduledValuesBeforeAllEvents) {
  auto param = TestableAudioParam(0.3, 0.0, 1.0, context);
  param.setValueAtTime(0.5, 0.1);
  param.setValueAtTime(0.8, 0.2);
  param.cancelScheduledValues(0.05);

  EXPECT_FLOAT_EQ(param.process(0.05), 0.3f);
  EXPECT_FLOAT_EQ(param.process(0.1), 0.3f);
  EXPECT_FLOAT_EQ(param.process(0.2), 0.3f);
}

// Cancel after all events is a no-op.
TEST_F(AudioParamTest, CancelScheduledValuesAfterAllEventsIsNoop) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.setValueAtTime(0.5, 0.1);
  param.cancelScheduledValues(0.5);

  EXPECT_FLOAT_EQ(param.process(0.1), 0.5f);
  EXPECT_FLOAT_EQ(param.process(0.3), 0.5f);
}

// Cancel at exactly an event's automationTime removes that event.
TEST_F(AudioParamTest, CancelScheduledValuesAtExactEventTime) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.setValueAtTime(0.5, 0.1);
  param.setValueAtTime(0.8, 0.2);
  param.cancelScheduledValues(0.2); // event at 0.1 survives, event at 0.2 removed

  EXPECT_FLOAT_EQ(param.process(0.1), 0.5f);
  EXPECT_FLOAT_EQ(param.process(0.2), 0.5f);
  EXPECT_FLOAT_EQ(param.process(0.3), 0.5f);
}

// Cancel at the curve's startTime (= automationTime) removes the curve.
TEST_F(AudioParamTest, CancelScheduledValuesAtCurveStart) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  auto curve = std::make_shared<AudioArray>(2);
  curve->span()[0] = 0.0f;
  curve->span()[1] = 1.0f;
  param.setValueCurveAtTime(curve, 2, 0.2, 0.2); // automationTime = startTime = 0.2
  param.cancelScheduledValues(0.2);

  EXPECT_FLOAT_EQ(param.process(0.2), 0.0f);
  EXPECT_FLOAT_EQ(param.process(0.4), 0.0f);
}

// Cancel mid-way through a curve's time range leaves the curve intact because
// automationTime (= startTime) is strictly before cancelTime.
TEST_F(AudioParamTest, CancelScheduledValuesWithinCurveRangeLeavesCurveIntact) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  auto curve = std::make_shared<AudioArray>(2);
  curve->span()[0] = 0.0f;
  curve->span()[1] = 1.0f;
  param.setValueCurveAtTime(curve, 2, 0.0, 0.4); // automationTime = 0.0
  param.setValueAtTime(0.5, 0.6);
  param.cancelScheduledValues(0.2); // curve survives; event at 0.6 removed

  EXPECT_FLOAT_EQ(param.process(0.0), 0.0f);
  EXPECT_FLOAT_EQ(param.process(0.4), 1.0f);
  EXPECT_FLOAT_EQ(param.process(0.6), 1.0f); // event gone — holds
}

// Cancel at a setTarget's startTime removes it.
TEST_F(AudioParamTest, CancelScheduledValuesAtSetTargetStart) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.setValueAtTime(0.5, 0.1);
  param.setTargetAtTime(1.0, 0.2, 0.1); // automationTime = startTime = 0.2
  param.cancelScheduledValues(0.2);     // removes setTarget; setValueAtTime survives

  EXPECT_FLOAT_EQ(param.process(0.1), 0.5f);
  EXPECT_FLOAT_EQ(param.process(0.2), 0.5f);
  EXPECT_FLOAT_EQ(param.process(0.5), 0.5f);
}

// Cancel after a setTarget's startTime does NOT remove it.
TEST_F(AudioParamTest, CancelScheduledValuesAfterSetTargetStartLeavesIt) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.setTargetAtTime(1.0, 0.0, 0.1);
  param.setValueAtTime(0.5, 0.3);
  param.cancelScheduledValues(0.2); // setTarget survives; event at 0.3 removed

  EXPECT_NEAR(param.process(0.1), 1.0f - std::exp(-1.0f), 1e-5f);
  // setTarget continues indefinitely
  EXPECT_NEAR(param.process(0.3), 1.0f - std::exp(-3.0f), 1e-5f);
}

// --- cancelAndHoldAtTime ---

// No events: no-op, value_ is returned.
TEST_F(AudioParamTest, CancelAndHoldAtTimeWithNoEvents) {
  auto param = TestableAudioParam(0.4, 0.0, 1.0, context);
  param.cancelAndHoldAtTime(0.5);

  EXPECT_FLOAT_EQ(param.process(0.5), 0.4f);
  EXPECT_FLOAT_EQ(param.process(1.0), 0.4f);
}

// E2 is a linear ramp: truncate it to end at cancelTime.
TEST_F(AudioParamTest, CancelAndHoldAtTime) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.setValueAtTime(0.8, 0.1);
  param.linearRampToValueAtTime(1.0, 0.2);
  // holdValue at t=0.15: 0.8 + (1.0 - 0.8) * 0.5 = 0.9
  param.cancelAndHoldAtTime(0.15);

  EXPECT_FLOAT_EQ(param.process(0.05), 0.0f);
  EXPECT_FLOAT_EQ(param.process(0.1), 0.8f);
  EXPECT_FLOAT_EQ(param.process(0.15), 0.9f);
  // Events after cancel time are removed -> stays at last value
  EXPECT_FLOAT_EQ(param.process(0.2), 0.9f);
  EXPECT_FLOAT_EQ(param.process(0.25), 0.9f);
}

// E2 is an exponential ramp: truncation exactly preserves values before cancelTime.
TEST_F(AudioParamTest, CancelAndHoldAtTimeWithExponentialRamp) {
  auto param = TestableAudioParam(0.1, 0.0, 1.0, context);
  // value(t) = 0.1 * 10^(t/0.2), holdValue = 0.1 * 10^0.5
  param.exponentialRampToValueAtTime(1.0, 0.2);
  param.cancelAndHoldAtTime(0.1);

  const float holdValue = 0.1f * std::pow(10.0f, 0.5f);

  EXPECT_NEAR(param.process(0.05), 0.1f * std::pow(10.0f, 0.25f), 1e-5f);
  EXPECT_NEAR(param.process(0.1), holdValue, 1e-5f);
  EXPECT_NEAR(param.process(0.3), holdValue, 1e-5f);
}

// E1 is setTarget (in queue): insert a setValueAtTime at cancelTime to freeze it.
TEST_F(AudioParamTest, CancelAndHoldAtTimeWithSetTarget) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  // value(t) = 1.0 - exp(-t/0.1), holdValue = 1.0 - exp(-1)
  param.setTargetAtTime(1.0, 0.0, 0.1);
  param.cancelAndHoldAtTime(0.1);

  const float holdValue = 1.0f - std::exp(-1.0f);

  EXPECT_NEAR(param.process(0.05), 1.0f - std::exp(-0.5f), 1e-5f);
  EXPECT_NEAR(param.process(0.1), holdValue, 1e-5f);
  EXPECT_NEAR(param.process(0.2), holdValue, 1e-5f);
  EXPECT_NEAR(param.process(1.0), holdValue, 1e-5f);
}

// E1 is setTarget already in currentEvent_ (drained from queue): still freezes correctly.
TEST_F(AudioParamTest, CancelAndHoldAtTimeWithCurrentEventSetTarget) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  param.setTargetAtTime(1.0, 0.0, 0.1);

  // Advance time to drain setTarget into currentEvent_
  EXPECT_NEAR(param.process(0.1), 1.0f - std::exp(-1.0f), 1e-5f);

  // holdValue at t=0.2: 1.0 - exp(-2)
  param.cancelAndHoldAtTime(0.2);

  const float holdValue = 1.0f - std::exp(-2.0f);

  EXPECT_NEAR(param.process(0.2), holdValue, 1e-5f);
  EXPECT_NEAR(param.process(0.5), holdValue, 1e-5f);
}

// E1 is setValueCurve within range: truncate curve at cancelTime.
// holdValue is computed using the original duration for sampling accuracy.
TEST_F(AudioParamTest, CancelAndHoldAtTimeWithSetValueCurveWithinRange) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  // 5-element linear curve [0, 0.25, 0.5, 0.75, 1.0] over [0.0, 0.4]
  auto curve = std::make_shared<AudioArray>(5);
  for (int i = 0; i < 5; ++i) {
    curve->span()[i] = i * 0.25f;
  }
  param.setValueCurveAtTime(curve, 5, 0.0, 0.4);
  // cancelTime = 0.2: k = floor(4/0.4 * 0.2) = 2 -> holdValue = curve[2] = 0.5
  param.cancelAndHoldAtTime(0.2);

  EXPECT_FLOAT_EQ(param.process(0.2), 0.5f);
  EXPECT_FLOAT_EQ(param.process(0.3), 0.5f);
  EXPECT_FLOAT_EQ(param.process(1.0), 0.5f);
}

// E1 is setValueCurve past its end: only removes later events, curve unmodified.
TEST_F(AudioParamTest, CancelAndHoldAtTimeWithSetValueCurveAfterRange) {
  auto param = TestableAudioParam(0.0, 0.0, 1.0, context);
  auto curve = std::make_shared<AudioArray>(2);
  curve->span()[0] = 0.0f;
  curve->span()[1] = 1.0f;
  param.setValueCurveAtTime(curve, 2, 0.0, 0.2); // curve ends at t=0.2
  param.setValueAtTime(0.5, 0.4);
  param.cancelAndHoldAtTime(0.3); // after curve end — event at 0.4 removed

  EXPECT_FLOAT_EQ(param.process(0.2), 1.0f);
  EXPECT_FLOAT_EQ(param.process(0.3), 1.0f);
  EXPECT_FLOAT_EQ(param.process(0.4), 1.0f); // event gone — holds
}

// NOLINTEND
