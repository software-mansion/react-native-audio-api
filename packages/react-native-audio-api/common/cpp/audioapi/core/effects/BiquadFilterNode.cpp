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
  channelCountMode_ = ChannelCountMode::MAX;
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
// frequency response -  H(z)
//          b0 + b1 * z^(-1) + b2 * z^(-2)
//  H(z) = -------------------------------
//           1 + a1 * z^(-1) + a2 * z^(-2)
//
//         b0 + (b1 + b2 * z1) * z1
//     =  --------------------------
//         (1 + (a1 + a2 * z1) * z1
//
// where z1 = 1/z and z = e^(j * pi * frequency)
// z1 = e^(-j * pi * frequency)
//
// phase response - angle of the frequency response
//

void BiquadFilterNode::getFrequencyResponse(
    const float *frequencyArray,
    float *magResponseOutput,
    float *phaseResponseOutput,
    const int length) {
  applyFilter();

  // Local copies for micro-optimization
  float b0 = b0_;
  float b1 = b1_;
  float b2 = b2_;
  float a1 = a1_;
  float a2 = a2_;

  for (size_t i = 0; i < length; i++) {
    if (frequencyArray[i] < 0.0 || frequencyArray[i] > 1.0) {
      magResponseOutput[i] = std::nanf("");
      phaseResponseOutput[i] = std::nanf("");
      continue;
    }

    auto omega = -PI * frequencyArray[i] / context_->getNyquistFrequency();
    auto z = std::complex<float>(cos(omega), sin(omega));
    auto response = (b0 + (b1 + b2 * z) * z) /
        (std::complex<float>(1, 0) + (a1 + a2 * z) * z);
    magResponseOutput[i] = static_cast<float>(std::abs(response));
    phaseResponseOutput[i] =
        static_cast<float>(atan2(imag(response), real(response)));
  }
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
  // Limit frequency to [0, 1] range
  if (frequency >= 1.0) {
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
  if (frequency >= 1.0) {
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
  float beta = (1 + cosW) / 2;

  setNormalizedCoefficients(
      beta, -2 * beta, beta, 1 + alpha, -2 * cosW, 1 - alpha);
}

void BiquadFilterNode::setBandpassCoefficients(float frequency, float Q) {
  // Limit frequency to [0, 1] range
  if (frequency <= 0.0 || frequency >= 1.0) {
    setNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  // Limit Q to positive values
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
  float A = std::pow(10.0f, gain / 40.0f);

  if (frequency >= 1.0) {
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
  float A = std::pow(10.0f, gain / 40.0f);

  if (frequency >= 1.0) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (frequency <= 0.0) {
    setNormalizedCoefficients(A * A, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = PI * frequency;
  // In the original formula: sqrt((A + 1/A) * (1/S - 1) + 2), but we assume
  // the maximum value S = 1, so it becomes 0 + 2 under the square root
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

  float frequency =
      frequencyParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
  float detune =
      detuneParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
  auto Q = QParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
  auto gain = gainParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);

  float normalizedFrequency = frequency / context_->getNyquistFrequency();
  if (detune != 0.0f) {
    normalizedFrequency *= std::pow(2.0f, detune / 1200.0f);
  }

  switch (type_) {
    case BiquadFilterType::LOWPASS:
      setLowpassCoefficients(normalizedFrequency, Q);
      break;
    case BiquadFilterType::HIGHPASS:
      setHighpassCoefficients(normalizedFrequency, Q);
      break;
    case BiquadFilterType::BANDPASS:
      setBandpassCoefficients(normalizedFrequency, Q);
      break;
    case BiquadFilterType::LOWSHELF:
      setLowshelfCoefficients(normalizedFrequency, gain);
      break;
    case BiquadFilterType::HIGHSHELF:
      setHighshelfCoefficients(normalizedFrequency, gain);
      break;
    case BiquadFilterType::PEAKING:
      setPeakingCoefficients(normalizedFrequency, Q, gain);
      break;
    case BiquadFilterType::NOTCH:
      setNotchCoefficients(normalizedFrequency, Q);
      break;
    case BiquadFilterType::ALLPASS:
      setAllpassCoefficients(normalizedFrequency, Q);
      break;
    default:
      break;
  }
}

void BiquadFilterNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  int numChannels = processingBus->getNumberOfChannels();

  applyFilter();

  // local copies for micro-optimization
  float b0 = b0_;
  float b1 = b1_;
  float b2 = b2_;
  float a1 = a1_;
  float a2 = a2_;

  float x1, x2, y1, y2;

  for (int c = 0; c < numChannels; ++c) {
    auto channelData = processingBus->getChannel(c)->getData();

    x1 = x1_;
    x2 = x2_;
    y1 = y1_;
    y2 = y2_;

    for (int i = 0; i < framesToProcess; ++i) {
      float input = channelData[i];
      float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

      channelData[i] = output;

      x2 = x1;
      x1 = input;
      y2 = y1;
      y1 = output;
    }
  }
  x1_ = x1;
  x2_ = x2;
  y1_ = y1;
  y2_ = y2;
}

} // namespace audioapi
