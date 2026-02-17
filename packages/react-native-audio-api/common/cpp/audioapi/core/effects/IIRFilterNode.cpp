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

#include <audioapi/types/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/IIRFilterNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

IIRFilterNode::IIRFilterNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const IIRFilterOptions &options)
    : AudioNode(context, options), feedforward_(options.feedforward), feedback_(options.feedback) {

  int maxChannels = MAX_CHANNEL_COUNT;
  xBuffers_.resize(maxChannels);
  yBuffers_.resize(maxChannels);
  bufferIndices.resize(maxChannels, 0);

  for (int c = 0; c < maxChannels; ++c) {
    xBuffers_[c].resize(bufferLength, 0.0f);
    yBuffers_[c].resize(bufferLength, 0.0f);
  }

  size_t feedforwardLength = feedforward_.size();
  size_t feedbackLength = feedback_.size();

  if (feedback_[0] != 1) {
    float scale = feedback_[0];
    for (unsigned k = 1; k < feedbackLength; ++k)
      feedback_[k] /= scale;

    for (unsigned k = 0; k < feedforwardLength; ++k)
      feedforward_[k] /= scale;

    feedback_[0] = 1.0f;
  }
  isInitialized_ = true;
}

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
    size_t length) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr)
    return;
  float nyquist = context->getNyquistFrequency();

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

    auto numerator = IIRFilterNode::evaluatePolynomial(feedforward_, z, feedforward_.size() - 1);
    auto denominator = IIRFilterNode::evaluatePolynomial(feedback_, z, feedback_.size() - 1);
    auto response = numerator / denominator;

    magResponseOutput[k] = static_cast<float>(std::abs(response));
    phaseResponseOutput[k] = static_cast<float>(atan2(imag(response), real(response)));
  }
}

// y[n] = sum(b[k] * x[n - k], k = 0, M) - sum(a[k] * y[n - k], k = 1, N)
// where b[k] are the feedforward coefficients and a[k] are the feedback coefficients of the filter

// TODO: tail

std::shared_ptr<AudioBuffer> IIRFilterNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  int numChannels = processingBuffer->getNumberOfChannels();

  size_t feedforwardLength = feedforward_.size();
  size_t feedbackLength = feedback_.size();
  int minLength = std::min(feedbackLength, feedforwardLength);

  int mask = bufferLength - 1;

  for (int c = 0; c < numChannels; ++c) {
    auto channel = processingBuffer->getChannel(c)->subSpan(framesToProcess);

    auto &x = xBuffers_[c];
    auto &y = yBuffers_[c];
    size_t bufferIndex = bufferIndices[c];

    for (float &sample : channel) {
      const float x_n = sample;
      float y_n = feedforward_[0] * sample;

      for (int k = 1; k < minLength; ++k) {
        int m = (bufferIndex - k) & mask;
        y_n = std::fma(feedforward_[k], x[m], y_n);
        y_n = std::fma(-feedback_[k], y[m], y_n);
      }

      for (int k = minLength; k < feedforwardLength; ++k) {
        y_n = std::fma(feedforward_[k], x[(bufferIndex - k) & mask], y_n);
      }
      for (int k = minLength; k < feedbackLength; ++k) {
        y_n = std::fma(-feedback_[k], y[(bufferIndex - k) & (bufferLength - 1)], y_n);
      }

      // Avoid denormalized numbers
      if (std::abs(y_n) < 1e-15f) {
        y_n = 0.0f;
      }

      sample = y_n;

      x[bufferIndex] = x_n;
      y[bufferIndex] = y_n;

      bufferIndex = (bufferIndex + 1) & (bufferLength - 1);
    }
    bufferIndices[c] = bufferIndex;
  }
  return processingBuffer;
}

} // namespace audioapi
