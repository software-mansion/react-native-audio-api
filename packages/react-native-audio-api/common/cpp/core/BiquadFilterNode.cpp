#include "BiquadFilterNode.h"
#include "BaseAudioContext.h"

// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html - math
// formulas for filters

namespace audioapi {

BiquadFilterNode::BiquadFilterNode(BaseAudioContext *context)
    : AudioNode(context) {
  frequencyParam_ = std::make_shared<AudioParam>(
      context, 350.0, MIN_FILTER_FREQUENCY, MAX_FILTER_FREQUENCY);
  detuneParam_ =
      std::make_shared<AudioParam>(context, 0.0, -MAX_DETUNE, MAX_DETUNE);
  QParam_ =
      std::make_shared<AudioParam>(context, 1.0, -MAX_FILTER_Q, MAX_FILTER_Q);
  gainParam_ = std::make_shared<AudioParam>(
      context, 0.0, MIN_FILTER_GAIN, MAX_FILTER_GAIN);
  type_ = BiquadFilterType::LOWPASS;
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
    const std::vector<float> &frequencyArray,
    std::vector<float> &magResponseOutput,
    std::vector<float> &phaseResponseOutput) {
  applyFilter();

  auto frequencyArraySize = frequencyArray.size();

  float b0 = b0_;
  float b1 = b1_;
  float b2 = b2_;
  float a1 = a1_;
  float a2 = a2_;

  for (size_t i = 0; i < frequencyArraySize; i++) {
    auto omega =
        static_cast<float>(M_PI) * frequencyArray[i] / NYQUIST_FREQUENCY;
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
  auto a0Inversed = 1.0f / a0;
  b0_ = b0 * a0Inversed;
  b1_ = b1 * a0Inversed;
  b2_ = b2 * a0Inversed;
  a1_ = a1 * a0Inversed;
  a2_ = a2 * a0Inversed;
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

  float theta = M_PI * frequency;
  float alpha = std::sin(theta) / (2 * g);
  float cosw = std::cos(theta);
  float beta = (1 - cosw) / 2;

  setNormalizedCoefficients(
      beta, 2 * beta, beta, 1 + alpha, -2 * cosw, 1 - alpha);
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

  float theta = M_PI * frequency;
  float alpha = std::sin(theta) / (2 * g);
  float cosw = std::cos(theta);
  float beta = (1 - cosw) / 2;

  setNormalizedCoefficients(
      beta, -2 * beta, beta, 1 + alpha, -2 * cosw, 1 - alpha);
}

void BiquadFilterNode::setBandpassCoefficients(float frequency, float Q) {
  frequency = std::clamp(frequency, 0.0f, 1.0f);
  Q = std::max(0.0f, Q);

  if (frequency <= 0.0 || frequency >= 1.0) {
    setNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (Q <= 0.0) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = M_PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float k = std::cos(w0);

  setNormalizedCoefficients(
      alpha, 0.0f, -alpha, 1.0f + alpha, -2 * k, 1.0f - alpha);
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

  float w0 = M_PI * frequency;
  float alpha =
      0.5f * std::sin(w0) * std::sqrt((A + 1 / A) * (1 / 1.0f - 1) + 2);
  float k = std::cos(w0);
  float k2 = 2.0f * std::sqrt(A) * alpha;

  setNormalizedCoefficients(
      A * (A + 1 - (A - 1) * k + k2),
      2.0f * A * (A - 1 - (A + 1) * k),
      A * (A + 1 - (A - 1) * k - k2),
      A + 1 + (A - 1) * k + k2,
      -2.0f * (A - 1 + (A + 1) * k),
      A + 1 + (A - 1) * k - k2);
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

  float w0 = M_PI * frequency;
  float alpha =
      0.5f * std::sin(w0) * std::sqrt((A + 1 / A) * (1 / 1.0f - 1) + 2);
  float k = std::cos(w0);
  float k2 = 2.0f * std::sqrt(A) * alpha;

  setNormalizedCoefficients(
      A * (A + 1 + (A - 1) * k + k2),
      -2.0f * A * (A - 1 + (A + 1) * k),
      A * (A + 1 + (A - 1) * k - k2),
      A + 1 - (A - 1) * k + k2,
      2.0f * (A - 1 - (A + 1) * k),
      A + 1 - (A - 1) * k - k2);
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

  float w0 = M_PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float k = std::cos(w0);

  setNormalizedCoefficients(
      1 + alpha * A,
      -2 * k,
      1 - alpha * A,
      1 + alpha / A,
      -2 * k,
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

  float w0 = M_PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float k = std::cos(w0);

  setNormalizedCoefficients(1.0f, -2 * k, 1.0f, 1 + alpha, -2 * k, 1 - alpha);
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

  float w0 = M_PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float k = std::cos(w0);

  setNormalizedCoefficients(
      1 - alpha, -2 * k, 1 + alpha, 1 + alpha, -2 * k, 1 - alpha);
}

void BiquadFilterNode::applyFilter() {
  double currentTime = context_->getCurrentTime();

  float normalizedFrequency =
      frequencyParam_->getValueAtTime(currentTime) / NYQUIST_FREQUENCY;
  float detuneValue = detuneParam_->getValueAtTime(currentTime);

  if (detuneValue != 0.0) {
    normalizedFrequency *= std::pow(2.0f, detuneValue / 1200.0f);
  }

  switch (type_) {
    case BiquadFilterType::LOWPASS:
      setLowpassCoefficients(
          normalizedFrequency, QParam_->getValueAtTime(currentTime));
      break;
    case BiquadFilterType::HIGHPASS:
      setHighpassCoefficients(
          normalizedFrequency, QParam_->getValueAtTime(currentTime));
      break;
    case BiquadFilterType::BANDPASS:
      setBandpassCoefficients(
          normalizedFrequency, QParam_->getValueAtTime(currentTime));
      break;
    case BiquadFilterType::LOWSHELF:
      setLowshelfCoefficients(
          normalizedFrequency, gainParam_->getValueAtTime(currentTime));
      break;
    case BiquadFilterType::HIGHSHELF:
      setHighshelfCoefficients(
          normalizedFrequency, gainParam_->getValueAtTime(currentTime));
      break;
    case BiquadFilterType::PEAKING:
      setPeakingCoefficients(
          normalizedFrequency,
          QParam_->getValueAtTime(currentTime),
          gainParam_->getValueAtTime(currentTime));
      break;
    case BiquadFilterType::NOTCH:
      setNotchCoefficients(
          normalizedFrequency, QParam_->getValueAtTime(currentTime));
      break;
    case BiquadFilterType::ALLPASS:
      setAllpassCoefficients(
          normalizedFrequency, QParam_->getValueAtTime(currentTime));
      break;
    default:
      break;
  }
}

bool BiquadFilterNode::processAudio(float *audioData, int32_t numFrames) {
  if (!AudioNode::processAudio(audioData, numFrames)) {
    return false;
  }

  resetCoefficients();
  applyFilter();

  float x1 = x1_;
  float x2 = x2_;
  float y1 = y1_;
  float y2 = y2_;

  float b0 = b0_;
  float b1 = b1_;
  float b2 = b2_;
  float a1 = a1_;
  float a2 = a2_;

  for (int i = 0; i < numFrames; i++) {
    auto input = audioData[i * channelCount_];
    auto output =
        static_cast<float>(b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2);

    for (int j = 0; j < channelCount_; j++) {
      audioData[i * channelCount_ + j] = output;
    }

    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
  }

  return true;
}
} // namespace audioapi
