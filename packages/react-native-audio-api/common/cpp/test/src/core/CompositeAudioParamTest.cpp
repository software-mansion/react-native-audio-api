#include <audioapi/core/AudioParam.h>
#include <audioapi/core/CompositeAudioParam.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <cmath>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

namespace {

float combineProduct(float a, float b) {
  return a * b;
}

float combineAmplitude(float a, float b) {
  return a * std::abs(b);
}

} // namespace

class CompositeAudioParamTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * sampleRate, sampleRate, eventRegistry);
  }

  std::shared_ptr<AudioParam> makeParam(float value) {
    return std::make_shared<AudioParam>(value, -100.0f, 100.0f, context);
  }
};

// --- Combine correctness ---

TEST_F(CompositeAudioParamTest, KRateCombine) {
  auto composite = std::make_shared<CompositeAudioParam<combineProduct>>(
      -1000.0f, 1000.0f, context, makeParam(3.0f), makeParam(4.0f));

  EXPECT_FLOAT_EQ(composite->processKRateParam(0.0), 12.0f);
}

TEST_F(CompositeAudioParamTest, ARateCombine) {
  auto composite = std::make_shared<CompositeAudioParam<combineProduct>>(
      -1000.0f, 1000.0f, context, makeParam(3.0f), makeParam(4.0f));

  auto out = composite->processARateParam(8, 0.0)->getChannel(0)->span();
  for (int i = 0; i < 8; ++i) {
    EXPECT_FLOAT_EQ(out[i], 12.0f);
  }
}

TEST_F(CompositeAudioParamTest, AmplitudeUsesAbsoluteValue) {
  auto composite = std::make_shared<CompositeAudioParam<combineAmplitude>>(
      -1000.0f, 1000.0f, context, makeParam(2.0f), makeParam(-3.0f));

  EXPECT_FLOAT_EQ(composite->processKRateParam(0.0), 6.0f);
}

// --- Nominal-range clamping ---

TEST_F(CompositeAudioParamTest, ClampsToNominalRange) {
  auto composite = std::make_shared<CompositeAudioParam<combineProduct>>(
      0.0f, 10.0f, context, makeParam(3.0f), makeParam(4.0f));

  EXPECT_FLOAT_EQ(composite->processKRateParam(0.0), 10.0f);

  auto out = composite->processARateParam(4, 1.0)->getChannel(0)->span();
  for (int i = 0; i < 4; ++i) {
    EXPECT_FLOAT_EQ(out[i], 10.0f);
  }
}

// --- Idempotency and child cache sharing ---

// Children can be processed (directly or via the composite) in any order and
// still share the per-quantum cache: modulation is consumed exactly once.
TEST_F(CompositeAudioParamTest, ChildrenProcessedInAnyOrderShareCache) {
  auto a = makeParam(2.0f);
  auto b = makeParam(5.0f);
  auto composite =
      std::make_shared<CompositeAudioParam<combineProduct>>(-1000.0f, 1000.0f, context, a, b);

  // Inject BridgeNode-style modulation into child a.
  a->getInputBuffer()->getChannel(0)->span()[0] = 1.0f;

  EXPECT_FLOAT_EQ(composite->processKRateParam(0.0), 15.0f);

  EXPECT_FLOAT_EQ(a->processKRateParam(0.0), 3.0f);
  EXPECT_FLOAT_EQ(b->processKRateParam(0.0), 5.0f);
  EXPECT_FLOAT_EQ(a->getInputBuffer()->getChannel(0)->span()[0], 0.0f);
}

TEST_F(CompositeAudioParamTest, KRateIsIdempotent) {
  auto a = makeParam(2.0f);
  auto b = makeParam(5.0f);
  auto composite =
      std::make_shared<CompositeAudioParam<combineProduct>>(-1000.0f, 1000.0f, context, a, b);

  a->getInputBuffer()->getChannel(0)->span()[0] = 1.0f;
  // Repeat call - cache hit, modulation not re-consumed.
  EXPECT_FLOAT_EQ(composite->processKRateParam(0.0), 15.0f);
  EXPECT_FLOAT_EQ(composite->processKRateParam(0.0), 15.0f);
}

TEST_F(CompositeAudioParamTest, ARateIsIdempotent) {
  auto a = makeParam(2.0f);
  auto b = makeParam(5.0f);
  auto composite =
      std::make_shared<CompositeAudioParam<combineProduct>>(-1000.0f, 1000.0f, context, a, b);

  auto aInput = a->getInputBuffer()->getChannel(0)->span();
  for (int i = 0; i < 4; ++i) {
    aInput[i] = 1.0f;
  }

  auto out1 = composite->processARateParam(4, 0.0)->getChannel(0)->span();
  EXPECT_FLOAT_EQ(out1[0], 15.0f);

  auto out2 = composite->processARateParam(4, 0.0)->getChannel(0)->span();
  EXPECT_FLOAT_EQ(out2[0], 15.0f);
}

// NOLINTEND
