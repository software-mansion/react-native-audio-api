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

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>

// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html - math
// formulas for filters
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers, readability-identifier-length)

namespace audioapi {

namespace {

constexpr double kPi = std::numbers::pi_v<double>;
constexpr double kSqrt2 = std::numbers::sqrt2_v<double>;

} // namespace

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
      computedFrequencyParam_(
          std::make_shared<CompositeAudioParam<combineBiquadFrequency>>(
              0.0f,
              getNyquistFrequency(),
              context,
              frequencyParam_,
              detuneParam_)),
      type_(options.type) {}

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
  const float frequency = frequencyParam_->getValue();
  const double Q = QParam_->getValue();
  const double gain = gainParam_->getValue();
  const float detune = detuneParam_->getValue();

  const auto coeffs = applyFilter(combineBiquadFrequency(frequency, detune), Q, gain, type);

  const double nyquist = getNyquistFrequency();

  for (size_t i = 0; i < length; i++) {
    // Convert from frequency in Hz to normalized frequency [0, 1]
    const double normalizedFreq = static_cast<double>(frequencyArray[i]) / nyquist;

    if (normalizedFreq < 0.0 || normalizedFreq > 1.0) {
      // Out-of-bounds frequencies should return NaN.
      magResponseOutput[i] = std::nanf("");
      phaseResponseOutput[i] = std::nanf("");
      continue;
    }

    const double omega = kPi * normalizedFreq;
    const double cosW = std::cos(omega);
    const double sinW = std::sin(omega);
    const double cos2W = std::cos(2.0 * omega);
    const double sin2W = std::sin(2.0 * omega);

    // Match the decomposed H(e^{j*pi*f}) evaluation used by the WPT reference
    // (biquad-getFrequencyResponse.html / biquad-filters.js) for better
    // numerical agreement near Nyquist than complex division.
    const double numeratorReal = coeffs.b0 + coeffs.b1 * cosW + coeffs.b2 * cos2W;
    const double numeratorImag = -(coeffs.b1 * sinW + coeffs.b2 * sin2W);
    const double denominatorReal = 1.0 + coeffs.a1 * cosW + coeffs.a2 * cos2W;
    const double denominatorImag = -(coeffs.a1 * sinW + coeffs.a2 * sin2W);

    const double magnitude = std::sqrt(
        (numeratorReal * numeratorReal + numeratorImag * numeratorImag) /
        (denominatorReal * denominatorReal + denominatorImag * denominatorImag));
    magResponseOutput[i] = static_cast<float>(magnitude);

    double phase =
        std::atan2(numeratorImag, numeratorReal) - std::atan2(denominatorImag, denominatorReal);
    if (phase >= kPi) {
      phase -= 2.0 * kPi;
    } else if (phase <= -kPi) {
      phase += 2.0 * kPi;
    }
    phaseResponseOutput[i] = static_cast<float>(phase);
  }
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getLowpassCoefficients(
    double frequency,
    double Q) {
  // Limit frequency to [0, 1] range
  if (frequency >= 1.0) {
    return getNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  if (frequency <= 0.0) {
    return getNormalizedCoefficients(0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  const double g = std::pow(10.0, 0.05 * Q);

  const double theta = kPi * frequency;
  const double alpha = std::sin(theta) / (2 * g);
  const double cosW = std::cos(theta);
  const double beta = (1 - cosW) / 2;

  return getNormalizedCoefficients(beta, 2 * beta, beta, 1 + alpha, -2 * cosW, 1 - alpha);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getHighpassCoefficients(
    double frequency,
    double Q) {
  if (frequency >= 1.0) {
    return getNormalizedCoefficients(0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }
  if (frequency <= 0.0) {
    return getNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  const double g = std::pow(10.0, 0.05 * Q);

  const double theta = kPi * frequency;
  const double alpha = std::sin(theta) / (2 * g);
  const double cosW = std::cos(theta);
  const double beta = (1 + cosW) / 2;

  return getNormalizedCoefficients(beta, -2 * beta, beta, 1 + alpha, -2 * cosW, 1 - alpha);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getBandpassCoefficients(
    double frequency,
    double Q) {
  // Limit frequency to [0, 1] range
  if (frequency <= 0.0 || frequency >= 1.0) {
    return getNormalizedCoefficients(0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  // Limit Q to positive values
  if (Q <= 0.0) {
    return getNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  const double w0 = kPi * frequency;
  const double alpha = std::sin(w0) / (2 * Q);
  const double cosW = std::cos(w0);

  return getNormalizedCoefficients(alpha, 0.0, -alpha, 1.0 + alpha, -2 * cosW, 1.0 - alpha);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getLowshelfCoefficients(
    double frequency,
    double gain) {
  const double A = std::pow(10.0, gain / 40.0);

  if (frequency >= 1.0) {
    return getNormalizedCoefficients(A * A, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  if (frequency <= 0.0) {
    return getNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  const double w0 = kPi * frequency;
  const double alpha = 0.5 * std::sin(w0) * kSqrt2;
  const double cosW = std::cos(w0);
  const double gamma = 2.0 * std::sqrt(A) * alpha;

  return getNormalizedCoefficients(
      A * (A + 1 - (A - 1) * cosW + gamma),
      2.0 * A * (A - 1 - (A + 1) * cosW),
      A * (A + 1 - (A - 1) * cosW - gamma),
      A + 1 + (A - 1) * cosW + gamma,
      -2.0 * (A - 1 + (A + 1) * cosW),
      A + 1 + (A - 1) * cosW - gamma);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getHighshelfCoefficients(
    double frequency,
    double gain) {
  const double A = std::pow(10.0, gain / 40.0);

  if (frequency >= 1.0) {
    return getNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  if (frequency <= 0.0) {
    return getNormalizedCoefficients(A * A, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  const double w0 = kPi * frequency;
  // In the original formula: sqrt((A + 1/A) * (1/S - 1) + 2), but we assume
  // the maximum value S = 1, so it becomes 0 + 2 under the square root
  const double alpha = 0.5 * std::sin(w0) * kSqrt2;
  const double cosW = std::cos(w0);
  const double gamma = 2.0 * std::sqrt(A) * alpha;

  return getNormalizedCoefficients(
      A * (A + 1 + (A - 1) * cosW + gamma),
      -2.0 * A * (A - 1 + (A + 1) * cosW),
      A * (A + 1 + (A - 1) * cosW - gamma),
      A + 1 - (A - 1) * cosW + gamma,
      2.0 * (A - 1 - (A + 1) * cosW),
      A + 1 - (A - 1) * cosW - gamma);
}

BiquadFilterNode::FilterCoefficients
BiquadFilterNode::getPeakingCoefficients(double frequency, double Q, double gain) {
  const double A = std::pow(10.0, gain / 40.0);

  if (frequency <= 0.0 || frequency >= 1.0) {
    return getNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  if (Q <= 0.0) {
    return getNormalizedCoefficients(A * A, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  const double w0 = kPi * frequency;
  const double alpha = std::sin(w0) / (2 * Q);
  const double cosW = std::cos(w0);

  return getNormalizedCoefficients(
      1 + alpha * A, -2 * cosW, 1 - alpha * A, 1 + alpha / A, -2 * cosW, 1 - alpha / A);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getNotchCoefficients(
    double frequency,
    double Q) {
  if (frequency <= 0.0 || frequency >= 1.0) {
    return getNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  if (Q <= 0.0) {
    return getNormalizedCoefficients(0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  const double w0 = kPi * frequency;
  const double alpha = std::sin(w0) / (2 * Q);
  const double cosW = std::cos(w0);

  return getNormalizedCoefficients(1.0, -2 * cosW, 1.0, 1 + alpha, -2 * cosW, 1 - alpha);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getAllpassCoefficients(
    double frequency,
    double Q) {
  if (frequency <= 0.0 || frequency >= 1.0) {
    return getNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  if (Q <= 0.0) {
    return getNormalizedCoefficients(-1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  }

  const double w0 = kPi * frequency;
  const double alpha = std::sin(w0) / (2 * Q);
  const double cosW = std::cos(w0);

  return getNormalizedCoefficients(
      1 - alpha, -2 * cosW, 1 + alpha, 1 + alpha, -2 * cosW, 1 - alpha);
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::getNormalizedCoefficients(
    double b0,
    double b1,
    double b2,
    double a0,
    double a1,
    double a2) {
  const double a0Inverted = 1.0 / a0;
  return {
      .b0 = b0 * a0Inverted,
      .b1 = b1 * a0Inverted,
      .b2 = b2 * a0Inverted,
      .a1 = a1 * a0Inverted,
      .a2 = a2 * a0Inverted};
}

BiquadFilterNode::FilterCoefficients BiquadFilterNode::applyFilter(
    double computedFrequency,
    double Q,
    double gain,
    BiquadFilterType type) {
  // `computedFrequency` already folds in detune (see `combineBiquadFrequency`).
  // NyquistFrequency is half of the sample rate.
  // Normalized frequency is therefore:
  // frequency / (sampleRate / 2) = (2 * frequency) / sampleRate
  double normalizedFrequency = computedFrequency / getNyquistFrequency();

  FilterCoefficients coeffs = {.b0 = 1.0, .b1 = 0.0, .b2 = 0.0, .a1 = 0.0, .a2 = 0.0};

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

void BiquadFilterNode::processNode(int framesToProcess) {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    const auto currentTime = context->getCurrentTime();

    // k-rate: sample the parameters once per render quantum and reuse the
    // resulting coefficients for every frame in the block.
    const float computedFrequency = computedFrequencyParam_->processKRateParam(currentTime);
    const float Q = QParam_->processKRateParam(currentTime);
    const float gain = gainParam_->processKRateParam(currentTime);

    const auto coeffs = applyFilter(computedFrequency, Q, gain, type_);

    lastA2_ = coeffs.a2;

    const auto numChannels = audioBuffer_->getNumberOfChannels();

    for (size_t c = 0; c < numChannels; ++c) {
      // Filter state kept in double precision for the recursive difference
      // equation; only the sample I/O is float.
      double x1 = x1_[c];
      double x2 = x2_[c];
      double y1 = y1_[c];
      double y2 = y2_[c];

      auto channel = audioBuffer_->getChannel(c)->subSpan(framesToProcess);

      for (float &sample : channel) {
        const double input = sample;
        double output =
            coeffs.b0 * input + coeffs.b1 * x1 + coeffs.b2 * x2 - coeffs.a1 * y1 - coeffs.a2 * y2;

        // Avoid denormalized numbers
        if (std::abs(output) < 1e-15) {
          output = 0.0;
        }

        sample = static_cast<float>(output);

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
  } else {
    audioBuffer_->zero();
  }
}

int BiquadFilterNode::computeTailFrames() const {
  // some ai math slop, but it matches web audio api, so I think it's correct
  // Two safety bounds:
  //   1. `r` is clamped to `1 - kPoleRadiusEpsilon` so that `log(r)` stays
  //      bounded away from zero for poles sitting arbitrarily close to the
  //      unit circle (very high Q).
  //   2. The returned frame count is capped at `kMaxTailSeconds * sr` so a
  //      pathological coefficient update cannot keep a node alive forever.
  const double rRaw = std::sqrt(std::abs(lastA2_));
  const double r = std::min(rRaw, 1.0 - kPoleRadiusEpsilon);
  if (r <= 0.0) {
    return 0;
  }
  const double frames = std::ceil(std::log(kTailEpsilon) / std::log(r));
  if (!std::isfinite(frames) || frames <= 0.0) {
    return 0;
  }
  const int cap = static_cast<int>(kMaxTailSeconds * getContextSampleRate());
  return std::min(static_cast<int>(frames), cap);
}

} // namespace audioapi

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers, readability-identifier-length)
