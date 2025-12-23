#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/IIRFilterNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <algorithm>
#include <complex>
#include <memory>
#include <numbers>
#include <vector>

using namespace audioapi;

class IIRFilterTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  static constexpr int sampleRate = 44100;
  static constexpr float nyquistFrequency = sampleRate / 2.0f;
  static constexpr float tolerance = 0.0001f;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * sampleRate, sampleRate, eventRegistry, RuntimeRegistry{});
  }

  static std::complex<double>
  evaluatePolynomial(std::span<const double> coefficients, std::complex<double> z, int order) {
    // Use Horner's method to evaluate the polynomial P(z) = sum(coef[k]*z^k, k, 0, order);
    std::complex<double> result = 0;
    for (int k = order; k >= 0; --k)
      result = result * z + std::complex<double>(coefficients[k]);
    return result;
  }

  static void getFrequencyResponseChromium(
      std::vector<float> feedforward,
      std::vector<float> feedback,
      unsigned length,
      std::span<const float> frequency,
      std::span<float> magResponse,
      std::span<float> phaseResponse,
      float nyquistFrequency) {
    assert(!frequency.empty());
    assert(!magResponse.empty());
    assert(!phaseResponse.empty());

    std::vector<double> m_feedforward(feedforward.begin(), feedforward.end());
    std::vector<double> m_feedback(feedback.begin(), feedback.end());

    std::vector<float> normalizedFreq(frequency.size());
    for (size_t i = 0; i < frequency.size(); ++i) {
      normalizedFreq[i] = frequency[i] / nyquistFrequency;
    }

    // Evaluate the z-transform of the filter at the given normalized frequencies
    // from 0 to 1. (One corresponds to the Nyquist frequency.)
    //
    // The z-tranform of the filter is
    //
    // H(z) = sum(b[k]*z^(-k), k, 0, M) / sum(a[k]*z^(-k), k, 0, N);
    //
    // The desired frequency response is H(exp(j*omega)), where omega is in [0,
    // 1).
    //
    // Let P(x) = sum(c[k]*x^k, k, 0, P) be a polynomial of order P. Then each of
    // the sums in H(z) is equivalent to evaluating a polynomial at the point
    // 1/z.

    for (unsigned k = 0; k < length; ++k) {
      if (normalizedFreq[k] < 0 || normalizedFreq[k] > 1) {
        // Out-of-bounds frequencies should return NaN.
        magResponse[k] = std::nanf("");
        phaseResponse[k] = std::nanf("");
      } else {
        // zRecip = 1/z = exp(-j*frequency)
        double omega = -std::numbers::pi * normalizedFreq[k];
        auto zRecip = std::complex<double>(cos(omega), sin(omega));

        auto numerator = evaluatePolynomial(m_feedforward, zRecip, m_feedforward.size() - 1);
        auto denominator = evaluatePolynomial(m_feedback, zRecip, m_feedback.size() - 1);
        auto response = numerator / denominator;
        magResponse[k] = static_cast<float>(std::abs(response));
        phaseResponse[k] = static_cast<float>(atan2(imag(response), real(response)));
      }
    }
  }
};

TEST_F(IIRFilterTest, IIRFilterCanBeCreated) {
  const std::vector<float> feedforward = {1.0};
  const std::vector<float> feedback = {1.0};
  auto node = context->createIIRFilter(feedforward, feedback);
  ASSERT_NE(node, nullptr);
}

TEST_F(IIRFilterTest, GetFrequencyResponse) {
  const std::vector<float> feedforward = {0.0050662636, 0.0101325272, 0.0050662636};
  const std::vector<float> feedback = {1.0632762845, -1.9797349456, 0.9367237155};

  auto node = IIRFilterNode(context, feedforward, feedback);

  float frequency = 1000.0f;
  float normalizedFrequency = frequency / nyquistFrequency;

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
  getFrequencyResponseChromium(
      feedforward,
      feedback,
      TestFrequencies.size(),
      TestFrequencies,
      magResponseExpected,
      phaseResponseExpected,
      nyquistFrequency);

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
