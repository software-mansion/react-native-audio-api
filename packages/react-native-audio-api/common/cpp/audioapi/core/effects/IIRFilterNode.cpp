/*
 * Copyright 2016 The Chromium Authors. All rights reserved.
 * Copyright (C) 2020 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/IIRFilterNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace audioapi {

IIRFilterNode::IIRFilterNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const IIRFilterOptions &options)
    : AudioNode(context, options),
      feedforward_(createNormalizedArray(options.feedforward, options.feedback[0])),
      feedback_(createNormalizedArray(options.feedback, options.feedback[0])),
      xBuffers_(BUFFER_LENGTH, MAX_CHANNEL_COUNT, context->getSampleRate()),
      yBuffers_(BUFFER_LENGTH, MAX_CHANNEL_COUNT, context->getSampleRate()),
      dominantPoleRadius_(computeDominantPoleRadius(feedback_)) {}

// Compute Z-transform of the filter
//
// frequency response -  H(z)
//          sum(b[k]*z^(-k), k, 0, M)
//  H(z) = -------------------------------
//           sum(a[k]*z^(-k), k, 0, N)
//
//          sum(b[k]*z1^k, k, 0, M)
//       = -------------------------------
//           sum(a[k]*z1^k, k, 0, N)
//
// where z1 = 1/z and z = e^(j * pi * frequency)
// z1 = e^(-j * pi * frequency)
//
// phase response - angle of the frequency response
//

void IIRFilterNode::getFrequencyResponse(
    const float *frequencyArray,
    float *magResponseOutput,
    float *phaseResponseOutput,
    size_t length) const {
  float nyquist = getNyquistFrequency();

  for (size_t k = 0; k < length; ++k) {
    float normalizedFreq = frequencyArray[k] / nyquist;

    if (normalizedFreq < 0.0f || normalizedFreq > 1.0f) {
      // Out-of-bounds frequencies should return NaN.
      magResponseOutput[k] = std::nanf("");
      phaseResponseOutput[k] = std::nanf("");
      continue;
    }

    float omega = -PI * normalizedFreq;
    auto z = std::complex<float>(std::cos(omega), std::sin(omega));

    auto numerator = IIRFilterNode::evaluatePolynomial(
        feedforward_, z, static_cast<int>(feedforward_.getSize() - 1));
    auto denominator =
        IIRFilterNode::evaluatePolynomial(feedback_, z, static_cast<int>(feedback_.getSize() - 1));
    auto response = numerator / denominator;

    magResponseOutput[k] = std::abs(response);
    phaseResponseOutput[k] = atan2(imag(response), real(response));
  }
}

// y[n] = sum(b[k] * x[n - k], k = 0, M) - sum(a[k] * y[n - k], k = 1, N)
// where b[k] are the feedforward coefficients and a[k] are the feedback coefficients of the filter

void IIRFilterNode::processNode(int framesToProcess) {
  int numChannels = static_cast<int>(audioBuffer_->getNumberOfChannels());

  size_t feedforwardLength = feedforward_.getSize();
  size_t feedbackLength = feedback_.getSize();
  auto minLength = static_cast<size_t>(std::min(feedbackLength, feedforwardLength));

  constexpr int mask = BUFFER_LENGTH - 1;

  for (int c = 0; c < numChannels; ++c) {
    auto channel = audioBuffer_->getChannel(c)->subSpan(framesToProcess);

    auto &x = xBuffers_[c];
    auto &y = yBuffers_[c];
    size_t bufferIndex = bufferIndices_[c];
    size_t k;

    for (float &sample : channel) {
      const float x_n = sample;
      float y_n = feedforward_[0] * sample;

      for (k = 1; k < minLength; ++k) {
        size_t m = (bufferIndex - k) & mask;
        y_n = std::fma(feedforward_[k], x[m], y_n);
        y_n = std::fma(-feedback_[k], y[m], y_n);
      }

      for (k = minLength; k < feedforwardLength; ++k) {
        y_n = std::fma(feedforward_[k], x[(bufferIndex - k) & mask], y_n);
      }
      for (k = minLength; k < feedbackLength; ++k) {
        y_n = std::fma(-feedback_[k], y[(bufferIndex - k) & (BUFFER_LENGTH - 1)], y_n);
      }

      // Avoid denormalized numbers
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
      if (std::abs(y_n) < 1e-15f) {
        y_n = 0.0f;
      }

      sample = y_n;

      x[bufferIndex] = x_n;
      y[bufferIndex] = y_n;

      bufferIndex = (bufferIndex + 1) & (BUFFER_LENGTH - 1);
    }
    bufferIndices_[c] = bufferIndex;
  }
}

double IIRFilterNode::computeDominantPoleRadius(const AudioArray &feedback) {
  // some ai math slop, but it matches web audio api, so I think it's correct
  //   1. run a short warmup so the iterate aligns with the dominant
  //      invariant subspace,
  //   2. take `exp(mean(log(norm_k)))` over a fixed measurement window.
  //
  // N is bounded by `BUFFER_LENGTH`, so the O(N^2) inner cost is trivial and
  // this runs once per filter construction.
  if (feedback.getSize() <= 1) {
    return 0.0;
  }
  const size_t n = feedback.getSize() - 1;

  std::vector<double> v(n, 0.0);
  std::vector<double> w(n, 0.0);
  v[0] = 1.0;

  // Performs w = C * v, writes back the normalized iterate into v and
  // returns ||w||. Returns 0 on a numerical collapse (extremely rare —
  // would require v to land exactly in the null space of C).
  auto step = [&]() -> double {
    // w = C * v. Companion row 0 pushes the negated feedback tail into w[0];
    // rows 1..n-1 are a simple shift from v[i-1].
    double top = 0.0;
    for (size_t k = 1; k <= n; ++k) {
      top -= static_cast<double>(feedback[k]) * v[k - 1];
    }
    w[0] = top;
    for (size_t i = 1; i < n; ++i) {
      w[i] = v[i - 1];
    }

    double normSq = 0.0;
    for (double x : w) {
      normSq += x * x;
    }
    const double norm = std::sqrt(normSq);
    if (norm == 0.0) {
      return 0.0;
    }
    const double invNorm = 1.0 / norm;
    for (size_t i = 0; i < n; ++i) {
      v[i] = w[i] * invNorm;
    }
    return norm;
  };

  constexpr int kWarmupIterations = 64;
  // Long enough that the log-mean of per-step norms is well within 1% of the
  // true `|λ|` even for the worst-case rotation angle (e.g. a Q=60 lowpass at
  // 1 kHz rotates by ≈ 8° per step, so 256 samples cover ≈ 5.7 full
  // revolutions — plenty for the oscillation to average out).
  constexpr int kMeasureIterations = 256;

  for (int i = 0; i < kWarmupIterations; ++i) {
    if (step() == 0.0) {
      return 0.0;
    }
  }

  double logSum = 0.0;
  for (int i = 0; i < kMeasureIterations; ++i) {
    const double norm = step();
    if (norm == 0.0) {
      return 0.0;
    }
    logSum += std::log(norm);
  }

  return std::exp(logSum / kMeasureIterations);
}

int IIRFilterNode::computeTailFrames() const {
  // Same model as BiquadFilterNode::computeTailFrames: the impulse envelope
  // decays at worst as `r^n` where `r` is the dominant pole magnitude of the
  // denominator polynomial. We precomputed that in `dominantPoleRadius_`.
  //
  //   1. `r` is clamped to `1 - kPoleRadiusEpsilon` so that `log(r)` stays
  //      bounded away from zero for poles arbitrarily close to the unit
  //      circle (very resonant filters).
  //   2. The resulting frame count is capped at `kMaxTailSeconds * sr` so a
  //      pathological coefficient set cannot keep the node alive forever.
  const double r = std::min(dominantPoleRadius_, 1.0 - kPoleRadiusEpsilon);
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
