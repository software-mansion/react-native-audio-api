/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/BiquadFilterNode.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <memory>
#include <string>

// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html - math
// formulas for filters

namespace audioapi {

BiquadFilterNode::BiquadFilterNode(std::shared_ptr<BaseAudioContext> context) : AudioNode(context) {
  frequencyParam_ =
      std::make_shared<AudioParam>(350.0, 0.0f, context->getNyquistFrequency(), context);
  detuneParam_ = std::make_shared<AudioParam>(
      0.0f,
      -1200 * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
      1200 * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
      context);
  QParam_ = std::make_shared<AudioParam>(
      1.0f, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);
  gainParam_ = std::make_shared<AudioParam>(
      0.0f, MOST_NEGATIVE_SINGLE_FLOAT, 40 * LOG10_MOST_POSITIVE_SINGLE_FLOAT, context);
  type_ = BiquadFilterType::LOWPASS;
  x1_.resize(MAX_CHANNEL_COUNT, 0.0f);
  x2_.resize(MAX_CHANNEL_COUNT, 0.0f);
  y1_.resize(MAX_CHANNEL_COUNT, 0.0f);
  y2_.resize(MAX_CHANNEL_COUNT, 0.0f);
  isInitialized_ = true;
  channelCountMode_ = ChannelCountMode::MAX;
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
    const size_t length) {
#if !RN_AUDIO_API_TEST
  applyFilter();
#endif

  // Use double precision for later calculations
  double b0 = static_cast<double>(b0_);
  double b1 = static_cast<double>(b1_);
  double b2 = static_cast<double>(b2_);
  double a1 = static_cast<double>(a1_);
  double a2 = static_cast<double>(a2_);

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (!context)
    return;
  float nyquist = context->getNyquistFrequency();

  for (size_t i = 0; i < length; i++) {
    // Convert from frequency in Hz to normalized frequency [0, 1]
    float normalizedFreq = frequencyArray[i] / nyquist;

    if (normalizedFreq < 0.0f || normalizedFreq > 1.0f) {
      // Out-of-bounds frequencies should return NaN.
      magResponseOutput[i] = std::nanf("");
      phaseResponseOutput[i] = std::nanf("");
      continue;
    }

    double omega = -PI * normalizedFreq;
    auto z = std::complex<double>(std::cos(omega), std::sin(omega));
    auto response = (b0 + (b1 + b2 * z) * z) / (std::complex<double>(1, 0) + (a1 + a2 * z) * z);
    magResponseOutput[i] = static_cast<float>(std::abs(response));
    phaseResponseOutput[i] = static_cast<float>(atan2(imag(response), real(response)));
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
  if (frequency >= 1.0f) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (frequency <= 0.0f) {
    setNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float g = std::pow(10.0f, 0.05f * Q);

  float theta = PI * frequency;
  float alpha = std::sin(theta) / (2 * g);
  float cosW = std::cos(theta);
  float beta = (1 - cosW) / 2;

  setNormalizedCoefficients(beta, 2 * beta, beta, 1 + alpha, -2 * cosW, 1 - alpha);
}

void BiquadFilterNode::setHighpassCoefficients(float frequency, float Q) {
  if (frequency >= 1.0f) {
    setNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }
  if (frequency <= 0.0f) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float g = std::pow(10.0f, 0.05f * Q);

  float theta = PI * frequency;
  float alpha = std::sin(theta) / (2 * g);
  float cosW = std::cos(theta);
  float beta = (1 + cosW) / 2;

  setNormalizedCoefficients(beta, -2 * beta, beta, 1 + alpha, -2 * cosW, 1 - alpha);
}

void BiquadFilterNode::setBandpassCoefficients(float frequency, float Q) {
  // Limit frequency to [0, 1] range
  if (frequency <= 0.0f || frequency >= 1.0f) {
    setNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  // Limit Q to positive values
  if (Q <= 0.0f) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  setNormalizedCoefficients(alpha, 0.0f, -alpha, 1.0f + alpha, -2 * cosW, 1.0f - alpha);
}

void BiquadFilterNode::setLowshelfCoefficients(float frequency, float gain) {
  float A = std::pow(10.0f, gain / 40.0f);

  if (frequency >= 1.0f) {
    setNormalizedCoefficients(A * A, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (frequency <= 0.0f) {
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

  if (frequency >= 1.0f) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (frequency <= 0.0f) {
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

void BiquadFilterNode::setPeakingCoefficients(float frequency, float Q, float gain) {
  float A = std::pow(10.0f, gain / 40.0f);

  if (frequency <= 0.0f || frequency >= 1.0f) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (Q <= 0.0f) {
    setNormalizedCoefficients(A * A, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  setNormalizedCoefficients(
      1 + alpha * A, -2 * cosW, 1 - alpha * A, 1 + alpha / A, -2 * cosW, 1 - alpha / A);
}

void BiquadFilterNode::setNotchCoefficients(float frequency, float Q) {
  if (frequency <= 0.0f || frequency >= 1.0f) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (Q <= 0.0f) {
    setNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  setNormalizedCoefficients(1.0f, -2 * cosW, 1.0f, 1 + alpha, -2 * cosW, 1 - alpha);
}

void BiquadFilterNode::setAllpassCoefficients(float frequency, float Q) {
  if (frequency <= 0.0f || frequency >= 1.0f) {
    setNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  if (Q <= 0.0f) {
    setNormalizedCoefficients(-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return;
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  setNormalizedCoefficients(1 - alpha, -2 * cosW, 1 + alpha, 1 + alpha, -2 * cosW, 1 - alpha);
}

void BiquadFilterNode::applyFilter() {
  // NyquistFrequency is half of the sample rate.
  // Normalized frequency is therefore:
  // frequency / (sampleRate / 2) = (2 * frequency) / sampleRate
  float normalizedFrequency;
  double currentTime;
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    currentTime = context->getCurrentTime();
    float frequency = frequencyParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
    normalizedFrequency = frequency / context->getNyquistFrequency();
  } else {
    return;
  }
  float detune = detuneParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
  auto Q = QParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
  auto gain = gainParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);

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

std::shared_ptr<AudioBus> BiquadFilterNode::processNode(
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

    x1 = x1_[c];
    x2 = x2_[c];
    y1 = y1_[c];
    y2 = y2_[c];

    for (int i = 0; i < framesToProcess; ++i) {
      float input = channelData[i];
      float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

      channelData[i] = output;

      x2 = x1;
      x1 = input;
      y2 = y1;
      y1 = output;
    }
    x1_[c] = x1;
    x2_[c] = x2;
    y1_[c] = y1;
    y2_[c] = y2;
  }

  return processingBus;
}

} // namespace audioapi
