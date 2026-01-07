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

#include <audioapi/dsp/Resampler.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <numbers>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

// based on WebKit UpSampler and DownSampler implementation

namespace audioapi {

Resampler::Resampler(int maxBlockSize, int kernelSize):
    kernelSize_(kernelSize),
    kernel_(std::make_shared<AudioArray>(kernelSize)),
    stateBuffer_(std::make_shared<AudioArray>(2 * maxBlockSize)) {
  stateBuffer_->zero();
}

// https://en.wikipedia.org/wiki/Window_function
float Resampler::computeBlackmanWindow(double x) const {
    double alpha = 0.16;
    double a0 = 0.5 * (1.0 - alpha);
    double a1 = 0.5;
    double a2 = 0.5 * alpha;
    double n = x / kernelSize_;
    return static_cast<float>(a0 - a1 * std::cos(2.0 * PI * n) + a2 * std::cos(4.0 * PI * n));
}

float Resampler::computeConvolution(const float *stateStart, const float *kernelStart) const {
  float sum = 0.0f;
  int k = 0;

#ifdef __ARM_NEON
  float32x4_t vSum = vdupq_n_f32(0.0f);

  // process 4 samples at a time
  for (; k <= kernelSize_ - 4; k += 4) {
    float32x4_t vState = vld1q_f32(stateStart + k);
    float32x4_t vKernel = vld1q_f32(kernelStart + k);

    // fused multiply-add: vSum += vState * vKernel
    vSum = vmlaq_f32(vSum, vState, vKernel);
  }

  // horizontal reduction: Sum the 4 lanes of vSum into a single float
  sum += vgetq_lane_f32(vSum, 0);
  sum += vgetq_lane_f32(vSum, 1);
  sum += vgetq_lane_f32(vSum, 2);
  sum += vgetq_lane_f32(vSum, 3);
#endif
  for (; k < kernelSize_; ++k) {
    sum += stateStart[k] * kernelStart[k];
  }

  return sum;
}

void Resampler::reset() {
  if (stateBuffer_) {
    stateBuffer_->zero();
  }
}

UpSampler::UpSampler(int maxBlockSize, int kernelSize) : Resampler(maxBlockSize, kernelSize) {
  initializeKernel();
}

void UpSampler::initializeKernel() {
  auto kData = kernel_->getData();
  int halfSize = kernelSize_ / 2;
  double subSampleOffset = -0.5;

  for (int i = 0; i < kernelSize_; ++i) {
    // we want to sample the sinc function halfway between integer points
    auto x = static_cast<double>(i - halfSize) - subSampleOffset;

    // https://en.wikipedia.org/wiki/Sinc_filter
    // sets cutoff frequency to nyquist
    double sinc = (std::abs(x) < 1e-9) ? 1.0 : std::sin(x * PI) / (x * PI);

    // apply window in order smooth out the edges, because sinc extends to infinity in both directions
    kData[i] = static_cast<float>(sinc * computeBlackmanWindow(i - subSampleOffset));
  }

  // reverse kernel to match convolution implementation
  std::reverse(kData, kData + kernelSize_);
}

int UpSampler::process(
    const std::shared_ptr<AudioArray> &input,
    const std::shared_ptr<AudioArray> &output,
    int framesToProcess) {

  const float *inputData = input->getData();
  float *outputData = output->getData();
  float *state = stateBuffer_->getData();
  const float *kernel = kernel_->getData();

  // copy new input [ HISTORY | NEW DATA ]
  std::memcpy(state + kernelSize_, inputData, framesToProcess * sizeof(float));

  int halfKernel = kernelSize_ / 2;

  for (int i = 0; i < framesToProcess; ++i) {
    // direct copy for even samples with half kernel latency compensation
    outputData[2 * i] = state[kernelSize_ + i - halfKernel];

    // convolution for odd samples
    // symmetric Linear Phase filter has latency of half kernel size
    outputData[2 * i + 1] = computeConvolution(&state[i + 1], kernel);
  }

  // move new data to history [ NEW DATA | EMPTY ]
  std::memmove(state, state + framesToProcess, kernelSize_ * sizeof(float));

  return framesToProcess * 2;
}

DownSampler::DownSampler(int maxBlockSize, int kernelSize) : Resampler(maxBlockSize, kernelSize) {
    initializeKernel();
}

void DownSampler::initializeKernel() {
  auto kData = kernel_->getData();
  int halfSize = kernelSize_ / 2;

  for (int i = 0; i < kernelSize_; ++i) {
    // we want to sample the sinc function halfway between integer points
    auto x = static_cast<double>(i - halfSize);

    // https://en.wikipedia.org/wiki/Sinc_filter
    // sets cutoff frequency to nyquist / 2
    double sinc = (std::abs(x) < 1e-9) ? 1.0 : std::sin(x * PI * 0.5) / (x * PI * 0.5);
    sinc *= 0.5;

    // apply window in order smooth out the edges, because sinc extends to infinity in both directions
    kData[i] = static_cast<float>(sinc * computeBlackmanWindow(i));
  }

  // reverse kernel to match convolution implementation
  std::reverse(kData, kData + kernelSize_);
}

int DownSampler::process(
    const std::shared_ptr<AudioArray> &input,
    const std::shared_ptr<AudioArray> &output,
    int framesToProcess) {
  const float *inputData = input->getData();
  float *outputData = output->getData();
  float *state = stateBuffer_->getData();
  const float *kernel = kernel_->getData();

  // copy new input [ HISTORY | NEW DATA ]
  std::memcpy(state + kernelSize_, inputData, framesToProcess * sizeof(float));

  auto outputCount = framesToProcess / 2;

  for (int i = 0; i < outputCount; ++i) {
    // convolution for downsampled samples
    outputData[i] = computeConvolution(&state[2 * i + 1], kernel);
  }

  // move new data to history [ NEW DATA | EMPTY ]
  std::memmove(state, state + framesToProcess, kernelSize_ * sizeof(float));

  return outputCount;
}
} // namespace audioapi
