#include <test/src/biquad/BiquadFilterChromium.h>
#include <test/src/biquad/BiquadFilterTest.h>
#include <memory>
#include <vector>

namespace audioapi {

void BiquadFilterTest::expectCoefficientsNear(
    const BiquadFilterNode &biquadNode,
    const BiquadCoefficients &expected) {
  EXPECT_NEAR(biquadNode.b0_, expected.b0, tolerance);
  EXPECT_NEAR(biquadNode.b1_, expected.b1, tolerance);
  EXPECT_NEAR(biquadNode.b2_, expected.b2, tolerance);
  EXPECT_NEAR(biquadNode.a1_, expected.a1, tolerance);
  EXPECT_NEAR(biquadNode.a2_, expected.a2, tolerance);
}

void BiquadFilterTest::testLowpass(float frequency, float Q) {
  auto node = BiquadFilterNode(context);
  float normalizedFrequency = frequency / nyquistFrequency;

  node.setLowpassCoefficients(normalizedFrequency, Q);
  expectCoefficientsNear(node, calculateLowpassCoefficients(normalizedFrequency, Q));
}

void BiquadFilterTest::testHighpass(float frequency, float Q) {
  auto node = BiquadFilterNode(context);
  float normalizedFrequency = frequency / nyquistFrequency;

  node.setHighpassCoefficients(normalizedFrequency, Q);
  expectCoefficientsNear(node, calculateHighpassCoefficients(normalizedFrequency, Q));
}

void BiquadFilterTest::testBandpass(float frequency, float Q) {
  auto node = BiquadFilterNode(context);
  float normalizedFrequency = frequency / nyquistFrequency;

  node.setBandpassCoefficients(normalizedFrequency, Q);
  expectCoefficientsNear(node, calculateBandpassCoefficients(normalizedFrequency, Q));
}

void BiquadFilterTest::testNotch(float frequency, float Q) {
  auto node = BiquadFilterNode(context);
  float normalizedFrequency = frequency / nyquistFrequency;

  node.setNotchCoefficients(normalizedFrequency, Q);
  expectCoefficientsNear(node, calculateNotchCoefficients(normalizedFrequency, Q));
}

void BiquadFilterTest::testAllpass(float frequency, float Q) {
  auto node = BiquadFilterNode(context);
  float normalizedFrequency = frequency / nyquistFrequency;

  node.setAllpassCoefficients(normalizedFrequency, Q);
  expectCoefficientsNear(node, calculateAllpassCoefficients(normalizedFrequency, Q));
}

void BiquadFilterTest::testPeaking(float frequency, float Q, float gain) {
  auto node = BiquadFilterNode(context);
  float normalizedFrequency = frequency / nyquistFrequency;

  node.setPeakingCoefficients(normalizedFrequency, Q, gain);
  expectCoefficientsNear(node, calculatePeakingCoefficients(normalizedFrequency, Q, gain));
}

void BiquadFilterTest::testLowshelf(float frequency, float gain) {
  auto node = BiquadFilterNode(context);
  float normalizedFrequency = frequency / nyquistFrequency;

  node.setLowshelfCoefficients(normalizedFrequency, gain);
  expectCoefficientsNear(node, calculateLowshelfCoefficients(normalizedFrequency, gain));
}

void BiquadFilterTest::testHighshelf(float frequency, float gain) {
  auto node = BiquadFilterNode(context);
  float normalizedFrequency = frequency / nyquistFrequency;

  node.setHighshelfCoefficients(normalizedFrequency, gain);
  expectCoefficientsNear(node, calculateHighshelfCoefficients(normalizedFrequency, gain));
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

TEST_P(BiquadFilterFrequencyTest, SetLowpassCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  testLowpass(frequency, Q);
}

TEST_P(BiquadFilterFrequencyTest, SetHighpassCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  testHighpass(frequency, Q);
}

TEST_P(BiquadFilterFrequencyTest, SetBandpassCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  testBandpass(frequency, Q);
}

TEST_P(BiquadFilterFrequencyTest, SetNotchCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  testNotch(frequency, Q);
}

TEST_P(BiquadFilterFrequencyTest, SetAllpassCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  testAllpass(frequency, Q);
}

TEST_P(BiquadFilterFrequencyTest, SetPeakingCoefficients) {
  float frequency = GetParam();
  float Q = 1.0f;
  float gain = 2.0f;
  testPeaking(frequency, Q, gain);
}

TEST_P(BiquadFilterFrequencyTest, SetLowshelfCoefficients) {
  float frequency = GetParam();
  float gain = 2.0f;
  testLowshelf(frequency, gain);
}

TEST_P(BiquadFilterFrequencyTest, SetHighshelfCoefficients) {
  float frequency = GetParam();
  float gain = 2.0f;
  testHighshelf(frequency, gain);
}

TEST_P(BiquadFilterQTestLowpassHighpass, SetLowpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testLowpass(frequency, Q);
}

TEST_P(BiquadFilterQTestLowpassHighpass, SetHighpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testHighpass(frequency, Q);
}

TEST_P(BiquadFilterQTestRestTypes, SetBandpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testBandpass(frequency, Q);
}

TEST_P(BiquadFilterQTestRestTypes, SetNotchCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testNotch(frequency, Q);
}

TEST_P(BiquadFilterQTestRestTypes, SetAllpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testAllpass(frequency, Q);
}

TEST_P(BiquadFilterQTestRestTypes, SetPeakingCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  float gain = 2.0f;
  testPeaking(frequency, Q, gain);
}

TEST_P(BiquadFilterGainTest, SetPeakingCoefficients) {
  float frequency = 1000.0f;
  float Q = 1.0f;
  float gain = GetParam();
  testPeaking(frequency, Q, gain);
}

TEST_P(BiquadFilterGainTest, SetLowshelfCoefficients) {
  float frequency = 1000.0f;
  float gain = GetParam();
  testLowshelf(frequency, gain);
}

TEST_P(BiquadFilterGainTest, SetHighshelfCoefficients) {
  float frequency = 1000.0f;
  float gain = GetParam();
  testHighshelf(frequency, gain);
}

TEST_F(BiquadFilterTest, GetFrequencyResponse) {
  auto node = BiquadFilterNode(context);

  float frequency = 1000.0f;
  float Q = 1.0f;
  float normalizedFrequency = frequency / nyquistFrequency;

  node.setLowpassCoefficients(normalizedFrequency, Q);
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
      TestFrequencies.size());
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
