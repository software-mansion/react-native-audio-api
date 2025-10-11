#include <test/src/biquad/BiquadFilterChromium.h>
#include <test/src/biquad/BiquadFilterTest.h>

namespace audioapi {

void BiquadFilterTest::expectCoefficientsEqual(
    const std::shared_ptr<BiquadFilterNode> &biquadNode,
    const BiquadCoefficients &expected) {
  EXPECT_FLOAT_EQ(biquadNode->b0_, expected.b0);
  EXPECT_FLOAT_EQ(biquadNode->b1_, expected.b1);
  EXPECT_FLOAT_EQ(biquadNode->b2_, expected.b2);
  EXPECT_FLOAT_EQ(biquadNode->a1_, expected.a1);
  EXPECT_FLOAT_EQ(biquadNode->a2_, expected.a2);
}

void BiquadFilterTest::testLowpass(float frequency, float Q) {
  auto node = std::make_shared<BiquadFilterNode>(context.get());
  node->setLowpassCoefficients(frequency, Q);
  expectCoefficientsEqual(node, calculateLowpassCoefficients(frequency, Q));
}

void BiquadFilterTest::testHighpass(float frequency, float Q) {
  auto node = std::make_shared<BiquadFilterNode>(context.get());
  node->setHighpassCoefficients(frequency, Q);
  expectCoefficientsEqual(node, calculateHighpassCoefficients(frequency, Q));
}

void BiquadFilterTest::testBandpass(float frequency, float Q) {
  auto node = std::make_shared<BiquadFilterNode>(context.get());
  node->setBandpassCoefficients(frequency, Q);
  expectCoefficientsEqual(node, calculateBandpassCoefficients(frequency, Q));
}

void BiquadFilterTest::testNotch(float frequency, float Q) {
  auto node = std::make_shared<BiquadFilterNode>(context.get());
  node->setNotchCoefficients(frequency, Q);
  expectCoefficientsEqual(node, calculateNotchCoefficients(frequency, Q));
}

void BiquadFilterTest::testAllpass(float frequency, float Q) {
  auto node = std::make_shared<BiquadFilterNode>(context.get());
  node->setAllpassCoefficients(frequency, Q);
  expectCoefficientsEqual(node, calculateAllpassCoefficients(frequency, Q));
}

void BiquadFilterTest::testPeaking(float frequency, float Q, float gain) {
  auto node = std::make_shared<BiquadFilterNode>(context.get());
  node->setPeakingCoefficients(frequency, Q, gain);
  expectCoefficientsEqual(
      node, calculatePeakingCoefficients(frequency, Q, gain));
}

void BiquadFilterTest::testLowshelf(float frequency, float gain) {
  auto node = std::make_shared<BiquadFilterNode>(context.get());
  node->setLowshelfCoefficients(frequency, gain);
  expectCoefficientsEqual(node, calculateLowshelfCoefficients(frequency, gain));
}

void BiquadFilterTest::testHighshelf(float frequency, float gain) {
  auto node = std::make_shared<BiquadFilterNode>(context.get());
  node->setHighshelfCoefficients(frequency, gain);
  expectCoefficientsEqual(
      node, calculateHighshelfCoefficients(frequency, gain));
}

INSTANTIATE_TEST_SUITE_P(
    Frequencies,
    BiquadFilterFrequencyTest,
    ::testing::Values(
        0.0f, // 0 Hz - the filter should block all input signal
        10.0f, // very low frequency
        350.0f, // default
        sampleRate / 2.0f - 0.0001f, // frequency near Nyquist
        sampleRate / 2.0f // Nyquist (maximal frequency)
        ));

INSTANTIATE_TEST_SUITE_P(
    QEdgeCases,
    BiquadFilterQTest,
    ::testing::Values(
        MOST_NEGATIVE_SINGLE_FLOAT,
        -770.63678f, // min value for lowpass and highpass
        0.0f, // default
        770.63678f, // max value for lowpass and highpass
        MOST_POSITIVE_SINGLE_FLOAT));

INSTANTIATE_TEST_SUITE_P(
    GainEdgeCases,
    BiquadFilterGainTest,
    ::testing::Values(
        MOST_NEGATIVE_SINGLE_FLOAT,
        -40.0f,
        0.0f, // default
        40.0f,
        40 * LOG10_MOST_POSITIVE_SINGLE_FLOAT));

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

TEST_P(BiquadFilterQTest, SetLowpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testLowpass(frequency, Q);
}

TEST_P(BiquadFilterQTest, SetHighpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testHighpass(frequency, Q);
}

TEST_P(BiquadFilterQTest, SetBandpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testBandpass(frequency, Q);
}

TEST_P(BiquadFilterQTest, SetNotchCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testNotch(frequency, Q);
}

TEST_P(BiquadFilterQTest, SetAllpassCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  testAllpass(frequency, Q);
}

TEST_P(BiquadFilterQTest, SetPeakingCoefficients) {
  float frequency = 1000.0f;
  float Q = GetParam();
  float gain = 2.0f;
  testPeaking(frequency, Q, gain);
}

TEST_P(BiquadFilterQTest, SetLowshelfCoefficients) {
  float frequency = 1000.0f;
  float gain = 2.0f;
  testLowshelf(frequency, gain);
}

TEST_P(BiquadFilterQTest, SetHighshelfCoefficients) {
  float frequency = 1000.0f;
  float gain = 2.0f;
  testHighshelf(frequency, gain);
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
  auto node = std::make_shared<BiquadFilterNode>(context.get());

  float cutoff = 1000.0f;
  float Q = 1.0f;
  node->setLowpassCoefficients(cutoff, Q);
  auto coeffs = calculateLowpassCoefficients(cutoff, Q);

  std::vector<float> normalizedTestFrequencies = {
      -0.00001f, 0.0f, 0.00001f, 0.25f, 0.5f, 0.75f, 1.0f, 1.00001f};

  std::vector<float> magResponseNode(normalizedTestFrequencies.size());
  std::vector<float> phaseResponseNode(normalizedTestFrequencies.size());
  std::vector<float> magResponseRef(normalizedTestFrequencies.size());
  std::vector<float> phaseResponseRef(normalizedTestFrequencies.size());

  node->getFrequencyResponse(
      normalizedTestFrequencies.data(),
      magResponseNode.data(),
      phaseResponseNode.data(),
      normalizedTestFrequencies.size());
  getFrequencyResponse(
      coeffs, normalizedTestFrequencies, magResponseRef, phaseResponseRef);

  for (size_t i = 0; i < normalizedTestFrequencies.size(); ++i) {
    float f = normalizedTestFrequencies[i];
    if (std::isnan(magResponseRef[i])) {
      EXPECT_TRUE(std::isnan(magResponseNode[i]))
          << "Expected NaN at frequency " << f;
    } else {
      EXPECT_EQ(magResponseNode[i], magResponseRef[i])
          << "Magnitude mismatch at " << f << " Hz";
    }

    if (std::isnan(phaseResponseRef[i])) {
      EXPECT_TRUE(std::isnan(phaseResponseNode[i]))
          << "Expected NaN at frequency " << f;
    } else {
      EXPECT_EQ(phaseResponseNode[i], phaseResponseRef[i])
          << "Phase mismatch at " << f << " Hz";
    }
  }
}

} // namespace audioapi
