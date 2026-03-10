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
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <memory>

// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html - math
// formulas for filters

namespace audioapi {

BiquadFilterNode::BiquadFilterNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const BiquadFilterOptions &options)
    : AudioNode(context, options),
      frequencyParam_(
          std::make_shared<AudioParam>(options.frequency, 0.0f, getNyquistFrequency(), context)),
      detuneParam_(
          std::make_shared<AudioParam>(
              options.detune,
              -OCTAVE_RANGE * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
              OCTAVE_RANGE * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
              context)),
      QParam_(
          std::make_shared<AudioParam>(
              options.Q,
              MOST_NEGATIVE_SINGLE_FLOAT,
              MOST_POSITIVE_SINGLE_FLOAT,
              context)),
      gainParam_(
          std::make_shared<AudioParam>(
              options.gain,
              MOST_NEGATIVE_SINGLE_FLOAT,
              BIQUAD_GAIN_DB_FACTOR * LOG10_MOST_POSITIVE_SINGLE_FLOAT,
              context)),
      type_(options.type),
      x1_(MAX_CHANNEL_COUNT),
      x2_(MAX_CHANNEL_COUNT),
      y1_(MAX_CHANNEL_COUNT),
      y2_(MAX_CHANNEL_COUNT) {
  isInitialized_.store(true, std::memory_order_release);
}

void BiquadFilterNode::setType(BiquadFilterType type) {
  type_ = type;
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
    const size_t length,
    BiquadFilterType type) {
  auto frequency = frequencyParam_->getValue();
  auto Q = QParam_->getValue();
  auto gain = gainParam_->getValue();
  auto detune = detuneParam_->getValue();

  auto coeffs = applyFilter(frequency, Q, gain, detune, type);

  float nyquist = getNyquistFrequency();

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
    auto response = (coeffs.b0 + (coeffs.b1 + coeffs.b2 * z) * z) /
        (std::complex<double>(1, 0) + (coeffs.a1 + coeffs.a2 * z) * z);
    magResponseOutput[i] = static_cast<float>(std::abs(response));
    phaseResponseOutput[i] = static_cast<float>(atan2(imag(response), real(response)));
  }
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getLowpassCoefficients(
    float frequency,
    float Q) {
  // Limit frequency to [0, 1] range
  if (frequency >= 1.0f) {
    return getNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  if (frequency <= 0.0f) {
    return getNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  float g = std::pow(10.0f, 0.05f * Q);

  float theta = PI * frequency;
  float alpha = std::sin(theta) / (2 * g);
  float cosW = std::cos(theta);
  float beta = (1 - cosW) / 2;

  return getNormalizedCoefficients(beta, 2 * beta, beta, 1 + alpha, -2 * cosW, 1 - alpha);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getHighpassCoefficients(
    float frequency,
    float Q) {
  if (frequency >= 1.0f) {
    return getNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }
  if (frequency <= 0.0f) {
    return getNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  float g = std::pow(10.0f, 0.05f * Q);

  float theta = PI * frequency;
  float alpha = std::sin(theta) / (2 * g);
  float cosW = std::cos(theta);
  float beta = (1 + cosW) / 2;

  return getNormalizedCoefficients(beta, -2 * beta, beta, 1 + alpha, -2 * cosW, 1 - alpha);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getBandpassCoefficients(
    float frequency,
    float Q) {
  // Limit frequency to [0, 1] range
  if (frequency <= 0.0f || frequency >= 1.0f) {
    return getNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  // Limit Q to positive values
  if (Q <= 0.0f) {
    return getNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  return getNormalizedCoefficients(alpha, 0.0f, -alpha, 1.0f + alpha, -2 * cosW, 1.0f - alpha);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getLowshelfCoefficients(
    float frequency,
    float gain) {
  float A = std::pow(10.0f, gain / 40.0f);

  if (frequency >= 1.0f) {
    return getNormalizedCoefficients(A * A, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  if (frequency <= 0.0f) {
    return getNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  float w0 = PI * frequency;
  float alpha = 0.5f * std::sin(w0) * std::sqrt(2.0f);
  float cosW = std::cos(w0);
  float gamma = 2.0f * std::sqrt(A) * alpha;

  return getNormalizedCoefficients(
      A * (A + 1 - (A - 1) * cosW + gamma),
      2.0f * A * (A - 1 - (A + 1) * cosW),
      A * (A + 1 - (A - 1) * cosW - gamma),
      A + 1 + (A - 1) * cosW + gamma,
      -2.0f * (A - 1 + (A + 1) * cosW),
      A + 1 + (A - 1) * cosW - gamma);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getHighshelfCoefficients(
    float frequency,
    float gain) {
  float A = std::pow(10.0f, gain / 40.0f);

  if (frequency >= 1.0f) {
    return getNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  if (frequency <= 0.0f) {
    return getNormalizedCoefficients(A * A, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  float w0 = PI * frequency;
  // In the original formula: sqrt((A + 1/A) * (1/S - 1) + 2), but we assume
  // the maximum value S = 1, so it becomes 0 + 2 under the square root
  float alpha = 0.5f * std::sin(w0) * std::sqrt(2.0f);
  float cosW = std::cos(w0);
  float gamma = 2.0f * std::sqrt(A) * alpha;

  return getNormalizedCoefficients(
      A * (A + 1 + (A - 1) * cosW + gamma),
      -2.0f * A * (A - 1 + (A + 1) * cosW),
      A * (A + 1 + (A - 1) * cosW - gamma),
      A + 1 - (A - 1) * cosW + gamma,
      2.0f * (A - 1 - (A + 1) * cosW),
      A + 1 - (A - 1) * cosW - gamma);
}

BiquadFilterNode::FilterCoefficients
BiquadFilterNode::getPeakingCoefficients(float frequency, float Q, float gain) {
  float A = std::pow(10.0f, gain / 40.0f);

  if (frequency <= 0.0f || frequency >= 1.0f) {
    return getNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  if (Q <= 0.0f) {
    return getNormalizedCoefficients(A * A, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  return getNormalizedCoefficients(
      1 + alpha * A, -2 * cosW, 1 - alpha * A, 1 + alpha / A, -2 * cosW, 1 - alpha / A);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getNotchCoefficients(
    float frequency,
    float Q) {
  if (frequency <= 0.0f || frequency >= 1.0f) {
    return getNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  if (Q <= 0.0f) {
    return getNormalizedCoefficients(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  return getNormalizedCoefficients(1.0f, -2 * cosW, 1.0f, 1 + alpha, -2 * cosW, 1 - alpha);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getAllpassCoefficients(
    float frequency,
    float Q) {
  if (frequency <= 0.0f || frequency >= 1.0f) {
    return getNormalizedCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  if (Q <= 0.0f) {
    return getNormalizedCoefficients(-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  }

  float w0 = PI * frequency;
  float alpha = std::sin(w0) / (2 * Q);
  float cosW = std::cos(w0);

  return getNormalizedCoefficients(
      1 - alpha, -2 * cosW, 1 + alpha, 1 + alpha, -2 * cosW, 1 - alpha);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getNormalizedCoefficients(
    float b0,
    float b1,
    float b2,
    float a0,
    float a1,
    float a2) {
  auto a0Inverted = 1.0f / a0;
  return {b0 * a0Inverted, b1 * a0Inverted, b2 * a0Inverted, a1 * a0Inverted, a2 * a0Inverted};
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::applyFilter(
    float frequency,
    float Q,
    float gain,
    float detune,
    BiquadFilterType type) {
  // NyquistFrequency is half of the sample rate.
  // Normalized frequency is therefore:
  // frequency / (sampleRate / 2) = (2 * frequency) / sampleRate
  float normalizedFrequency = frequency / getNyquistFrequency();

  if (detune != 0.0f) {
    normalizedFrequency *= std::pow(2.0f, detune / 1200.0f);
  }

  FilterCoefficients coeffs = {1.0, 0.0, 0.0, 0.0, 0.0};

  switch (type) {
    case BiquadFilterType::LOWPASS:
      coeffs = getLowpassCoefficients(normalizedFrequency, Q);
      break;
    case BiquadFilterType::HIGHPASS:
      coeffs = getHighpassCoefficients(normalizedFrequency, Q);
      break;
    case BiquadFilterType::BANDPASS:
      coeffs = getBandpassCoefficients(normalizedFrequency, Q);
      break;
    case BiquadFilterType::LOWSHELF:
      coeffs = getLowshelfCoefficients(normalizedFrequency, gain);
      break;
    case BiquadFilterType::HIGHSHELF:
      coeffs = getHighshelfCoefficients(normalizedFrequency, gain);
      break;
    case BiquadFilterType::PEAKING:
      coeffs = getPeakingCoefficients(normalizedFrequency, Q, gain);
      break;
    case BiquadFilterType::NOTCH:
      coeffs = getNotchCoefficients(normalizedFrequency, Q);
      break;
    case BiquadFilterType::ALLPASS:
      coeffs = getAllpassCoefficients(normalizedFrequency, Q);
      break;
    default:
      break;
  }

  return coeffs;
}

std::shared_ptr<AudioBuffer> BiquadFilterNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    auto currentTime = context->getCurrentTime();
    float frequency = frequencyParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
    float detune = detuneParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
    auto Q = QParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);
    auto gain = gainParam_->processKRateParam(RENDER_QUANTUM_SIZE, currentTime);

    auto coeffs = applyFilter(frequency, Q, gain, detune, type_);

    float x1, x2, y1, y2;

    auto numChannels = processingBuffer->getNumberOfChannels();

    for (size_t c = 0; c < numChannels; ++c) {
      auto channel = processingBuffer->getChannel(c)->subSpan(framesToProcess);

      x1 = x1_[c];
      x2 = x2_[c];
      y1 = y1_[c];
      y2 = y2_[c];

      for (float &sample : channel) {
        auto input = sample;
        auto output =
            coeffs.b0 * input + coeffs.b1 * x1 + coeffs.b2 * x2 - coeffs.a1 * y1 - coeffs.a2 * y2;

        // Avoid denormalized numbers
        if (std::abs(output) < 1e-15f) {
          output = 0.0f;
        }

        sample = static_cast<float>(output);

        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = static_cast<float>(output);
      }

      x1_[c] = x1;
      x2_[c] = x2;
      y1_[c] = y1;
      y2_[c] = y2;
    }
  } else {
    processingBuffer->zero();
  }

  return processingBuffer;
}

} // namespace audioapi
