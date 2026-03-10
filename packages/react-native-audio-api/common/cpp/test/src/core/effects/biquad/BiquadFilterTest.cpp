#include <audioapi/types/NodeOptions.h>
#include <test/src/core/effects/biquad/BiquadFilterChromium.h>
#include <test/src/core/effects/biquad/BiquadFilterTest.h>
#include <memory>
#include <vector>

namespace audioapi {

void BiquadFilterTest::expectCoefficientsNear(
    const BiquadFilterNode::FilterCoefficients &actual,
    const BiquadCoefficients &expected) {
  EXPECT_NEAR(actual.b0, expected.b0, tolerance);
  EXPECT_NEAR(actual.b1, expected.b1, tolerance);
  EXPECT_NEAR(actual.b2, expected.b2, tolerance);
  EXPECT_NEAR(actual.a1, expected.a1, tolerance);
  EXPECT_NEAR(actual.a2, expected.a2, tolerance);
}

void BiquadFilterTest::testLowpass(float frequency, float Q) {
  float normalizedFrequency = frequency / nyquistFrequency;
  auto coeffs = BiquadFilterNode::getLowpassCoefficients(normalizedFrequency, Q);
  expectCoefficientsNear(coeffs, calculateLowpassCoefficients(normalizedFrequency, Q));
}

void BiquadFilterTest::testHighpass(float frequency, float Q) {
  float normalizedFrequency = frequency / nyquistFrequency;
  auto coeffs = BiquadFilterNode::getHighpassCoefficients(normalizedFrequency, Q);
  expectCoefficientsNear(coeffs, calculateHighpassCoefficients(normalizedFrequency, Q));
}

void BiquadFilterTest::testBandpass(float frequency, float Q) {
  float normalizedFrequency = frequency / nyquistFrequency;
  auto coeffs = BiquadFilterNode::getBandpassCoefficients(normalizedFrequency, Q);
  expectCoefficientsNear(coeffs, calculateBandpassCoefficients(normalizedFrequency, Q));
}

void BiquadFilterTest::testNotch(float frequency, float Q) {
  float normalizedFrequency = frequency / nyquistFrequency;
  auto coeffs = BiquadFilterNode::getNotchCoefficients(normalizedFrequency, Q);
  expectCoefficientsNear(coeffs, calculateNotchCoefficients(normalizedFrequency, Q));
}

void BiquadFilterTest::testAllpass(float frequency, float Q) {
  float normalizedFrequency = frequency / nyquistFrequency;
  auto coeffs = BiquadFilterNode::getAllpassCoefficients(normalizedFrequency, Q);
  expectCoefficientsNear(coeffs, calculateAllpassCoefficients(normalizedFrequency, Q));
}

void BiquadFilterTest::testPeaking(float frequency, float Q, float gain) {
  float normalizedFrequency = frequency / nyquistFrequency;
  auto coeffs = BiquadFilterNode::getPeakingCoefficients(normalizedFrequency, Q, gain);
  expectCoefficientsNear(coeffs, calculatePeakingCoefficients(normalizedFrequency, Q, gain));
}

void BiquadFilterTest::testLowshelf(float frequency, float gain) {
  float normalizedFrequency = frequency / nyquistFrequency;
  auto coeffs = BiquadFilterNode::getLowshelfCoefficients(normalizedFrequency, gain);
  expectCoefficientsNear(coeffs, calculateLowshelfCoefficients(normalizedFrequency, gain));
}

void BiquadFilterTest::testHighshelf(float frequency, float gain) {
  float normalizedFrequency = frequency / nyquistFrequency;
  auto coeffs = BiquadFilterNode::getHighshelfCoefficients(normalizedFrequency, gain);
  expectCoefficientsNear(coeffs, calculateHighshelfCoefficients(normalizedFrequency, gain));
}

INSTANTIATE_TEST_SUITE_P(
    Frequencies,
    BiquadFilterFrequencyTest,
    ::testing::Values(
        0.0f,                       // 0 Hz - the filter should block all input signal
        10.0f,                      // very low frequency
        350.0f,                     // default
        nyquistFrequency - 0.0001f, // frequency near Nyquist
        nyquistFrequency));         // maximal frequency

INSTANTIATE_TEST_SUITE_P(
    QEdgeCases,
    BiquadFilterQTestLowpassHighpass,
    ::testing::Values(
        -770.63678f,  // min value for lowpass and highpass
        0.0f,         // default
        770.63678f)); // max value for lowpass and highpass

INSTANTIATE_TEST_SUITE_P(
    QEdgeCases,
    BiquadFilterQTestRestTypes, // bandpass, notch, allpass, peaking
    ::testing::Values(
        0.0f, // default and min value
        MOST_POSITIVE_SINGLE_FLOAT));

INSTANTIATE_TEST_SUITE_P(
    GainEdgeCases,
    BiquadFilterGainTest,
    ::testing::Values(
        -40.0f,
        0.0f, // default
        40.0f));

TEST_P(BiquadFilterFrequencyTest, TestLowpassCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  testLowpass(frequency, Q);
}

TEST_P(BiquadFilterFrequencyTest, TestHighpassCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  testHighpass(frequency, Q);
}

TEST_P(BiquadFilterFrequencyTest, TestBandpassCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  testBandpass(frequency, Q);
}

TEST_P(BiquadFilterFrequencyTest, TestNotchCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  testNotch(frequency, Q);
}

TEST_P(BiquadFilterFrequencyTest, TestAllpassCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  testAllpass(frequency, Q);
}

TEST_P(BiquadFilterFrequencyTest, TestPeakingCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  float gain = 2.0f;
  testPeaking(frequency, Q, gain);
}

TEST_P(BiquadFilterFrequencyTest, TestLowshelfCoefficients) {
  float frequency = GetParam();
  float gain = 2.0f;
  testLowshelf(frequency, gain);
}

TEST_P(BiquadFilterFrequencyTest, TestHighshelfCoefficients) {
  float frequency = GetParam();
  float gain = 2.0f;
  testHighshelf(frequency, gain);
}

TEST_P(BiquadFilterQTestLowpassHighpass, TestLowpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testLowpass(frequency, Q);
}

TEST_P(BiquadFilterQTestLowpassHighpass, TestHighpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testHighpass(frequency, Q);
}

TEST_P(BiquadFilterQTestRestTypes, TestBandpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testBandpass(frequency, Q);
}

TEST_P(BiquadFilterQTestRestTypes, TestNotchCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testNotch(frequency, Q);
}

TEST_P(BiquadFilterQTestRestTypes, TestAllpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testAllpass(frequency, Q);
}

TEST_P(BiquadFilterQTestRestTypes, TestPeakingCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  float gain = 2.0f;
  testPeaking(frequency, Q, gain);
}

TEST_P(BiquadFilterGainTest, TestPeakingCoefficients) {
  float frequency = 1000.0f;
  float Q = 1.0f;
  float gain = GetParam();
  testPeaking(frequency, Q, gain);
}

TEST_P(BiquadFilterGainTest, TestLowshelfCoefficients) {
  float frequency = 1000.0f;
  float gain = GetParam();
  testLowshelf(frequency, gain);
}

TEST_P(BiquadFilterGainTest, TestHighshelfCoefficients) {
  float frequency = 1000.0f;
  float gain = GetParam();
  testHighshelf(frequency, gain);
}

TEST_F(BiquadFilterTest, GetFrequencyResponse) {
  auto node = BiquadFilterNode(context, BiquadFilterOptions());

  float frequency = 1000.0f;
  float Q = 1.0f;
  float normalizedFrequency = frequency / nyquistFrequency;

  node.frequencyParam_->setValue(frequency);
  node.QParam_->setValue(Q);
  auto coeffs = calculateLowpassCoefficients(normalizedFrequency, Q);

  std::vector<float> TestFrequencies = {
      -0.0001f,
      0.0f,
      0.0001f,
      0.25f * nyquistFrequency,
      0.5f * nyquistFrequency,
      0.75f * nyquistFrequency,
      nyquistFrequency - 0.0001f,
      nyquistFrequency,
      nyquistFrequency + 0.0001f};

  std::vector<float> magResponseNode(TestFrequencies.size());
  std::vector<float> phaseResponseNode(TestFrequencies.size());
  std::vector<float> magResponseExpected(TestFrequencies.size());
  std::vector<float> phaseResponseExpected(TestFrequencies.size());

  node.getFrequencyResponse(
      TestFrequencies.data(),
      magResponseNode.data(),
      phaseResponseNode.data(),
      TestFrequencies.size(),
      BiquadFilterType::LOWPASS);
  getFrequencyResponse(
      coeffs, TestFrequencies, magResponseExpected, phaseResponseExpected, nyquistFrequency);

  for (size_t i = 0; i < TestFrequencies.size(); ++i) {
    float f = TestFrequencies[i];
    if (std::isnan(magResponseExpected[i])) {
      EXPECT_TRUE(std::isnan(magResponseNode[i])) << "Expected NaN at frequency " << f;
    } else {
      EXPECT_NEAR(magResponseNode[i], magResponseExpected[i], tolerance)
          << "Magnitude mismatch at " << f << " Hz";
    }

    if (std::isnan(phaseResponseExpected[i])) {
      EXPECT_TRUE(std::isnan(phaseResponseNode[i])) << "Expected NaN at frequency " << f;
    } else {
      EXPECT_NEAR(phaseResponseNode[i], phaseResponseExpected[i], tolerance)
          << "Phase mismatch at " << f << " Hz";
    }
  }
}

} // namespace audioapi
