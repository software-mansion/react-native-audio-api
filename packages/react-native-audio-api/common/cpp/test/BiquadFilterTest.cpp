#include <test/BiquadFilterTest.h>

namespace audioapi {

struct BiquadCoefficients {
  float b0;
  float b1;
  float b2;
  float a1;
  float a2;
};

BiquadCoefficients normalizeCoefficients(float a0, BiquadCoefficients coeffs) {
  return {
      coeffs.b0 / a0,
      coeffs.b1 / a0,
      coeffs.b2 / a0,
      coeffs.a1 / a0,
      coeffs.a2 / a0,
  };
}

BiquadCoefficients
calculateLowpassCoefficients(float frequency, float Q, float sampleRate) {
  float omega = 2.0 * M_PI * frequency / sampleRate;
  float alphaQdb = std::sin(omega) / (2.0 * std::pow(10.0f, Q / 20.0f));

  float b0 = (1.0 - std::cos(omega)) / 2.0;
  float b1 = 1.0 - std::cos(omega);
  float b2 = (1.0 - std::cos(omega)) / 2.0;
  float a0 = 1.0 + alphaQdb;
  float a1 = -2.0 * std::cos(omega);
  float a2 = 1.0 - alphaQdb;

  // BiquadFilterNode stores normalized coefficients because
  // the biquad transfer function requires them in that form
  return normalizeCoefficients(a0, {b0, b1, b2, a1, a2});
}

BiquadCoefficients
calculateHighpassCoefficients(float frequency, float Q, float sampleRate) {
  float omega = 2.0 * M_PI * frequency / sampleRate;
  float alphaQdb = std::sin(omega) / (2.0 * Q);

  float b0 = (1.0 + std::cos(omega)) / 2.0;
  float b1 = -(1.0 + std::cos(omega));
  float b2 = (1.0 + std::cos(omega)) / 2.0;
  float a0 = 1.0 + alphaQdb;
  float a1 = -2.0 * std::cos(omega);
  float a2 = 1.0 - alphaQdb;

  return normalizeCoefficients(a0, {b0, b1, b2, a1, a2});
}

BiquadCoefficients
calculateBandpassCoefficients(float frequency, float Q, float sampleRate) {
  float omega = 2.0f * M_PI * frequency / sampleRate;
  float alphaQ = std::sin(omega) / (2.0f * Q);

  float b0 = alphaQ;
  float b1 = 0.0f;
  float b2 = -alphaQ;
  float a0 = 1.0f + alphaQ;
  float a1 = -2.0f * std::cos(omega);
  float a2 = 1.0f - alphaQ;

  return normalizeCoefficients(a0, {b0, b1, b2, a1, a2});
}

BiquadCoefficients
calculateNotchCoefficients(float frequency, float Q, float sampleRate) {
  float omega = 2.0f * M_PI * frequency / sampleRate;
  float alphaQ = std::sin(omega) / (2.0f * Q);

  float b0 = 1.0f;
  float b1 = -2.0f * std::cos(omega);
  float b2 = 1.0f;
  float a0 = 1.0f + alphaQ;
  float a1 = -2.0f * std::cos(omega);
  float a2 = 1.0f - alphaQ;

  return normalizeCoefficients(a0, {b0, b1, b2, a1, a2});
}

BiquadCoefficients
calculateAllpassCoefficients(float frequency, float Q, float sampleRate) {
  float omega = 2.0f * M_PI * frequency / sampleRate;
  float alphaQ = std::sin(omega) / (2.0f * Q);

  float b0 = 1.0f - alphaQ;
  float b1 = -2.0f * std::cos(omega);
  float b2 = 1.0f + alphaQ;
  float a0 = 1.0f + alphaQ;
  float a1 = -2.0f * std::cos(omega);
  float a2 = 1.0f - alphaQ;

  return normalizeCoefficients(a0, {b0, b1, b2, a1, a2});
}

BiquadCoefficients calculatePeakingCoefficients(
    float frequency,
    float Q,
    float gainDB,
    float sampleRate) {
  float omega = 2.0f * M_PI * frequency / sampleRate;
  float alphaQ = std::sin(omega) / (2.0f * Q);
  float A = std::pow(10.0f, gainDB / 40.0f);

  float b0 = 1.0f + alphaQ * A;
  float b1 = -2.0f * std::cos(omega);
  float b2 = 1.0f - alphaQ * A;
  float a0 = 1.0f + alphaQ / A;
  float a1 = -2.0f * std::cos(omega);
  float a2 = 1.0f - alphaQ / A;

  return normalizeCoefficients(a0, {b0, b1, b2, a1, a2});
}

BiquadCoefficients
calculateLowshelfCoefficients(float frequency, float gainDB, float sampleRate) {
  float omega = 2.0f * M_PI * frequency / sampleRate;
  float A = std::pow(10.0f, gainDB / 40.0f);
  float S = 1.0f;
  float alphaS = std::sin(omega) / 2.0f *
      std::sqrt((A + (1.0f / A)) * ((1.0f / S) - 1) + 2.0f);

  float b0 = A *
      ((A + 1.0f) - (A - 1.0f) * std::cos(omega) +
       2.0f * alphaS * std::sqrt(A));
  float b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * std::cos(omega));
  float b2 = A *
      ((A + 1.0f) - (A - 1.0f) * std::cos(omega) -
       2.0f * alphaS * std::sqrt(A));
  float a0 =
      (A + 1.0f) + (A - 1.0f) * std::cos(omega) + 2.0f * alphaS * std::sqrt(A);
  float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * std::cos(omega));
  float a2 =
      (A + 1.0f) + (A - 1.0f) * std::cos(omega) - 2.0f * alphaS * std::sqrt(A);

  return normalizeCoefficients(a0, {b0, b1, b2, a1, a2});
}

BiquadCoefficients calculateHighshelfCoefficients(
    float frequency,
    float gainDB,
    float sampleRate) {
  float omega = 2.0f * M_PI * frequency / sampleRate;
  float A = std::pow(10.0f, gainDB / 40.0f);
  float S = 1.0f;
  float alphaS = std::sin(omega) / 2.0f *
      std::sqrt((A + (1.0f / A)) * ((1.0f / S) - 1) + 2.0f);

  float b0 = A *
      ((A + 1.0f) - (A - 1.0f) * std::cos(omega) +
       2.0f * alphaS * std::sqrt(A));
  float b1 = -2.0f * A * ((A - 1.0f) - (A + 1.0f) * std::cos(omega));
  float b2 = A *
      ((A + 1.0f) - (A - 1.0f) * std::cos(omega) -
       2.0f * alphaS * std::sqrt(A));
  float a0 =
      (A + 1.0f) + (A - 1.0f) * std::cos(omega) + 2.0f * alphaS * std::sqrt(A);
  float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * std::cos(omega));
  float a2 =
      (A + 1.0f) + (A - 1.0f) * std::cos(omega) - 2.0f * alphaS * std::sqrt(A);

  return normalizeCoefficients(a0, {b0, b1, b2, a1, a2});
}

TEST_F(BiquadFilterTest, TestLowpassCoefficients) {
  float frequency = 1000.0f;
  float Q = 1.0f;
  float sampleRate = context->getSampleRate();
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setLowpassCoefficients(frequency, Q);
  auto coeffs = calculateLowpassCoefficients(frequency, Q, sampleRate);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestHighpassCoefficients) {
  float frequency = 500.0f;
  float Q = 0.75f;
  float sampleRate = context->getSampleRate();
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setHighpassCoefficients(frequency, Q);
  auto coeffs = calculateHighpassCoefficients(frequency, Q, sampleRate);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestBandpassCoefficients) {
  float frequency = 22000.0f;
  float Q = -20.4f;
  float sampleRate = context->getSampleRate();
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setBandpassCoefficients(frequency, Q);
  auto coeffs = calculateBandpassCoefficients(frequency, Q, sampleRate);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestNotchCoefficients) {
  float frequency = 1000.0f;
  float Q = 10.0f;
  float sampleRate = context->getSampleRate();
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setNotchCoefficients(frequency, Q);
  auto coeffs = calculateNotchCoefficients(frequency, Q, sampleRate);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestAllpassCoefficients) {
  float frequency = 1000.0f;
  float Q = 1.0f;
  float sampleRate = context->getSampleRate();
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setAllpassCoefficients(frequency, Q);
  auto coeffs = calculateAllpassCoefficients(frequency, Q, sampleRate);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestPeakingCoefficients) {
  float frequency = 1000.0f;
  float Q = 1.0f;
  float gainDB = 3.0f;
  float sampleRate = context->getSampleRate();
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setPeakingCoefficients(frequency, Q, gainDB);
  auto coeffs = calculatePeakingCoefficients(frequency, Q, gainDB, sampleRate);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestLowshelfCoefficients) {
  float frequency = 1000.0f;
  float gainDB = 3.0f;
  float sampleRate = context->getSampleRate();
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setLowshelfCoefficients(frequency, gainDB);
  auto coeffs = calculateLowshelfCoefficients(frequency, gainDB, sampleRate);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestHighshelfCoefficients) {
  float frequency = 1000.0f;
  float gainDB = 3.0f;
  float sampleRate = context->getSampleRate();
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setHighshelfCoefficients(frequency, gainDB);
  auto coeffs = calculateHighshelfCoefficients(frequency, gainDB, sampleRate);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}
} // namespace audioapi
