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

#pragma once

#include <audioapi/core/AudioNode.h>
#include <complex>
#include <vector>

#include <memory>

namespace audioapi {

class IIRFilterNode : public AudioNode {

 public:
  explicit IIRFilterNode(
      std::shared_ptr<BaseAudioContext> context,
      const std::vector<float> &feedforward,
      const std::vector<float> &feedback);

  void getFrequencyResponse(
      const float *frequencyArray,
      float *magResponseOutput,
      float *phaseResponseOutput,
      size_t length);

 protected:
  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;

 private:
  static constexpr size_t bufferLength = 32;

  std::vector<float> feedforward_;
  std::vector<float> feedback_;

  std::vector<std::vector<float>> xBuffers_; // xBuffers_[channel][index]
  std::vector<std::vector<float>> yBuffers_;
  std::vector<size_t> bufferIndices;

  static std::complex<float>
  evaluatePolynomial(const std::vector<float> coefficients, std::complex<float> z, int order) {
    // Use Horner's method to evaluate the polynomial P(z) = sum(coef[k]*z^k, k, 0, order);
    std::complex<float> result = 0;
    for (int k = order; k >= 0; --k)
      result = result * z + std::complex<float>(coefficients[k]);
    return result;
  }
};
} // namespace audioapi
