#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/BiquadFilterNode.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html - math
// formulas for filters

namespace audioapi {

BiquadFilterNode::BiquadFilterNode(BaseAudioContext *context)
    : AudioNode(context) {
  frequencyParam_ = std::make_shared<AudioParam>(
      350.0, 0.0f, context->getNyquistFrequency(), context);
  detuneParam_ = std::make_shared<AudioParam>(
      0.0,
      -1200 * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
      1200 * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
      context);
  QParam_ = std::make_shared<AudioParam>(
      1.0, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);
  gainParam_ = std::make_shared<AudioParam>(
      0.0,
      MOST_NEGATIVE_SINGLE_FLOAT,
      40 * LOG10_MOST_POSITIVE_SINGLE_FLOAT,
      context);
  type_ = BiquadFilterType::LOWPASS;
  isInitialized_ = true;
}

std::string BiquadFilterNode::getType() {
  return BiquadFilterNode::toString(type_);
}

void BiquadFilterNode::setType(const std::string &type) {
  type_ = BiquadFilterNode::fromString(type);
}

std::shared_ptr<AudioParam> BiquadFilterNode::getFrequencyParam() const {
  return frequencyParam_;
}

std::shared_ptr<AudioParam> BiquadFilterNode::getDetuneParam() const {
  return detuneParam_;
}

std::shared_ptr<AudioParam> BiquadFilterNode::getQParam() const {
  return QParam_;
}

std::shared_ptr<AudioParam> BiquadFilterNode::getGainParam() const {
  return gainParam_;
}

// Compute Z-transform of the filter
// https://www.dsprelated.com/freebooks/filters/Frequency_Response_Analysis.html
// https://www.dsprelated.com/freebooks/filters/Transfer_Function_Analysis.html
//
// frequency response -  H(z) = (b0 + b1*z^(-1) + b2*z^(-2))/(1 + a1*z^(-1) +
// a2*z^(-2)) = ((b0 * z + b1) * z + b2) / ((z + a1) * z + a2) phase response -
// angle of the frequency response

void BiquadFilterNode::getFrequencyResponse(
    const float *frequencyArray,
    float *magResponseOutput,
    float *phaseResponseOutput,
    const int length) {
  applyFilter();

  float b0 = b0_;
  float b1 = b1_;
  float b2 = b2_;
  float a1 = a1_;
  float a2 = a2_;

  for (size_t i = 0; i < length; i++) {
    auto omega = PI * frequencyArray[i] / context_->getNyquistFrequency();
    auto z = std::complex<float>(cos(omega), sin(omega));
    auto response = ((b0 * z + b1) * z + b2) / ((z + a1) * z + a2);
    magResponseOutput[i] = static_cast<float>(abs(response));
    phaseResponseOutput[i] =
        static_cast<float>(atan2(imag(response), real(response)));
  }
}

void BiquadFilterNode::resetCoefficients() {
  x1_ = 0.0;
  x2_ = 0.0;
  y1_ = 0.0;
  y2_ = 0.0;
}

void BiquadFilterNode::setNormalizedCoefficients(
    float b0,
    float b1,
    float b2,
    float a0,
    float a1,
    float a2) {
  auto a0Inverted = 1.0f / a0;
  b0_ = b0 * a0Inverted;
  b1_ = b1 * a0Inverted;
  b2_ = b2 * a0Inverted;
  a1_ = a1 * a0Inverted;
  a2_ = a2 * a0Inverted;
}

void BiquadFilterNode::setLowpassCoefficients(float frequency, float Q) {
  frequency = std::clamp(frequency, 0.0f, 1.0f);
  if (frequency == 1.0) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (frequency <= 0.0) {
    setNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  Q = std::max(0.0f, Q);
  float g = std::pow(10.0f, 0.05f * Q);

  float theta = PI * frequency;
  float alpha = std::sin(theta) / (2 * g);
  float cosW = std::cos(theta);
  float beta = (1 - cosW) / 2;

  setNormalizedCoefficients(
      beta, 2 * beta, beta, 1 + alpha, -2 * cosW, 1 - alpha);
}

void BiquadFilterNode::setHighpassCoefficients(float frequency, float Q) {
  frequency = std::clamp(frequency, 0.0f, 1.0f);
  if (frequency == 1.0) {
    setNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }
  if (frequency <= 0.0) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  Q = std::max(0.0f, Q);
  float g = std::pow(10.0f, 0.05f * Q);

  float theta = PI * frequency;
  float alpha = std::sin(theta) / (2 * g);
  float cosW = std::cos(theta);
  float beta = (1 - cosW) / 2;

  setNormalizedCoefficients(
      beta, -2 * beta, beta, 1 + alpha, -2 * cosW, 1 - alpha);
}

void BiquadFilterNode::setBandpassCoefficients(float frequency, float Q) {
  frequency = std::clamp(frequency, 0.0f, 1.0f);
  Q = std::max(0.0f, Q);

  // frequency == 1 jak w reszcie funkcji
  if (frequency <= 0.0 || frequency >= 1.0) {
    setNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  // Q == 0? Q nigdy nie jest ujemne
  if (Q <= 0.0) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  setNormalizedCoefficients(
      alpha, 0.0f, -alpha, 1.0f + alpha, -2 * cosW, 1.0f - alpha);
}

void BiquadFilterNode::setLowshelfCoefficients(float frequency, float gain) {
  frequency = std::clamp(frequency, 0.0f, 1.0f);
  float A = std::pow(10.0f, gain / 40.0f);

  if (frequency == 1.0) {
    setNormalizedCoefficients(A * A, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (frequency <= 0.0) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = PI * frequency;
  float alpha = 0.5f * std::sin(w0) * std::sqrt(2.0f);
  float cosW = std::cos(w0);
  float gamma = 2.0f * std::sqrt(A) * alpha;

  setNormalizedCoefficients(
      A * (A + 1 - (A - 1) * cosW + gamma),
      2.0f * A * (A - 1 - (A + 1) * cosW),
      A * (A + 1 - (A - 1) * cosW - gamma),
      A + 1 + (A - 1) * cosW + gamma,
      -2.0f * (A - 1 + (A + 1) * cosW),
      A + 1 + (A - 1) * cosW - gamma);
}

void BiquadFilterNode::setHighshelfCoefficients(float frequency, float gain) {
  frequency = std::clamp(frequency, 0.0f, 1.0f);
  float A = std::pow(10.0f, gain / 40.0f);

  if (frequency == 1.0) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (frequency <= 0.0) {
    setNormalizedCoefficients(A * A, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = PI * frequency;
  // w oryginalnym wzorze: sqrt((A + 1/A) * (1/S - 1) + 2), ale przyjmujemy
  // maksymalna wartosc S=1, wiec wychodzi 0+2 pod pierwiastkiem
  float alpha = 0.5f * std::sin(w0) * std::sqrt(2.0f);
  float cosW = std::cos(w0);
  float gamma = 2.0f * std::sqrt(A) * alpha;

  setNormalizedCoefficients(
      A * (A + 1 + (A - 1) * cosW + gamma),
      -2.0f * A * (A - 1 + (A + 1) * cosW),
      A * (A + 1 + (A - 1) * cosW - gamma),
      A + 1 - (A - 1) * cosW + gamma,
      2.0f * (A - 1 - (A + 1) * cosW),
      A + 1 - (A - 1) * cosW - gamma);
}

void BiquadFilterNode::setPeakingCoefficients(
    float frequency,
    float Q,
    float gain) {
  frequency = std::clamp(frequency, 0.0f, 1.0f);
  Q = std::max(0.0f, Q);
  float A = std::pow(10.0f, gain / 40.0f);

  if (frequency <= 0.0 || frequency >= 1.0) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (Q <= 0.0) {
    setNormalizedCoefficients(A * A, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  setNormalizedCoefficients(
      1 + alpha * A,
      -2 * cosW,
      1 - alpha * A,
      1 + alpha / A,
      -2 * cosW,
      1 - alpha / A);
}

void BiquadFilterNode::setNotchCoefficients(float frequency, float Q) {
  frequency = std::clamp(frequency, 0.0f, 1.0f);
  Q = std::max(0.0f, Q);

  if (frequency <= 0.0 || frequency >= 1.0) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (Q <= 0.0) {
    setNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  setNormalizedCoefficients(
      1.0f, -2 * cosW, 1.0f, 1 + alpha, -2 * cosW, 1 - alpha);
}

void BiquadFilterNode::setAllpassCoefficients(float frequency, float Q) {
  frequency = std::clamp(frequency, 0.0f, 1.0f);
  Q = std::max(0.0f, Q);

  if (frequency <= 0.0 || frequency >= 1.0) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (Q <= 0.0) {
    setNormalizedCoefficients(-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  setNormalizedCoefficients(
      1 - alpha, -2 * cosW, 1 + alpha, 1 + alpha, -2 * cosW, 1 - alpha);
}

void BiquadFilterNode::applyFilter() {
  double currentTime = context_->getCurrentTime();

  float frequencyParamValue =
      frequencyParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
  float normalizedFrequency =
      frequencyParamValue / context_->getNyquistFrequency();

  float detuneValue =
      detuneParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);

  if (detuneValue != 0.0) {
    normalizedFrequency *= std::pow(2.0f, detuneValue / 1200.0f);
  }

  auto qparamValue =
      QParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
  auto gainParamValue =
      gainParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
  switch (type_) {
    case BiquadFilterType::LOWPASS:
      setLowpassCoefficients(normalizedFrequency, qparamValue);
      break;
    case BiquadFilterType::HIGHPASS:
      setHighpassCoefficients(normalizedFrequency, qparamValue);
      break;
    case BiquadFilterType::BANDPASS:
      setBandpassCoefficients(normalizedFrequency, qparamValue);
      break;
    case BiquadFilterType::LOWSHELF:
      setLowshelfCoefficients(normalizedFrequency, gainParamValue);
      break;
    case BiquadFilterType::HIGHSHELF:
      setHighshelfCoefficients(normalizedFrequency, gainParamValue);
      break;
    case BiquadFilterType::PEAKING:
      setPeakingCoefficients(normalizedFrequency, qparamValue, gainParamValue);
      break;
    case BiquadFilterType::NOTCH:
      setNotchCoefficients(normalizedFrequency, qparamValue);
      break;
    case BiquadFilterType::ALLPASS:
      setAllpassCoefficients(normalizedFrequency, qparamValue);
      break;
    default:
      break;
  }
}

void BiquadFilterNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  resetCoefficients();
  applyFilter();

  for (int c = 0; c < processingBus->getNumberOfChannels(); c++) {
    float x1 = x1_;
    float x2 = x2_;
    float y1 = y1_;
    float y2 = y2_;

    float b0 = b0_;
    float b1 = b1_;
    float b2 = b2_;
    float a1 = a1_;
    float a2 = a2_;

    for (int i = 0; i < framesToProcess; i++) {
      float input = (*processingBus->getChannel(c))[i];
      float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

      (*processingBus->getChannel(c))[i] = output;

      x2 = x1;
      x1 = input;
      y2 = y1;
      y1 = output;
    }
  }
}

} // namespace audioapi
