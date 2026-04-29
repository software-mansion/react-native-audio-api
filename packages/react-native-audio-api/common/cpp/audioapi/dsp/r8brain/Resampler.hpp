#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <array>
#include <cstddef>
#include <memory>
#include <vector>

// Sample rate converter designed by Aleksey Vaneev of Voxengo on MIT license
#include "CDSPResampler.h"

namespace r8b {

class BaseResampler {
 public:
  BaseResampler(const BaseResampler &) = delete;
  BaseResampler &operator=(const BaseResampler &) = delete;
  BaseResampler(BaseResampler &&) noexcept = default;
  BaseResampler &operator=(BaseResampler &&) noexcept = default;
  virtual ~BaseResampler() = default;

  /** @return Maximum number of output samples per channel for one process() call (for the maxInLen passed to the constructor). */
  [[nodiscard]] int getMaxOutLen() const {
    if (resamplers_.empty()) {
      return 0;
    }
    return resamplers_[0]->getMaxOutLen(0);
  }

 private:
  std::vector<std::unique_ptr<CDSPResampler24>> resamplers_;
  std::vector<std::vector<double>> inputBuffers_;

 protected:
  static constexpr int DEFAULT_MAX_IN_LEN = 2048;

  BaseResampler(
      double srcRate,
      double dstRate,
      int numChannels,
      int maxInLen = DEFAULT_MAX_IN_LEN) {
    resamplers_.reserve(static_cast<size_t>(numChannels));
    inputBuffers_.reserve(static_cast<size_t>(numChannels));
    for (int i = 0; i < numChannels; ++i) {
      resamplers_.emplace_back(std::make_unique<CDSPResampler24>(srcRate, dstRate, maxInLen));
      inputBuffers_.emplace_back(static_cast<size_t>(maxInLen));
    }
  }

  // Allocation-free: callers pass raw pointer arrays so the resampler can be
  // driven from real-time / audio threads without heap traffic per call.
  int process(const float *const *input, int l, float *const *output) {
    int outLen = 0;
    const size_t numChannels = resamplers_.size();

    for (size_t i = 0; i < numChannels; ++i) {
      const float *__restrict inData = input[i];
      double *__restrict bufData = inputBuffers_[i].data();

      for (int j = 0; j < l; ++j) {
        bufData[j] = static_cast<double>(inData[j]);
      }

      double *outPtr = nullptr;
      const int currentOutLen = resamplers_[i]->process(bufData, l, outPtr);
      outLen = currentOutLen;

      if (currentOutLen > 0 && outPtr != nullptr) {
        const double *__restrict resampledData = outPtr;
        float *__restrict outData = output[i];

        for (int j = 0; j < currentOutLen; ++j) {
          outData[j] = static_cast<float>(resampledData[j]);
        }
      }
    }

    return outLen;
  }
};

class MultiChannelResampler : public BaseResampler {
 public:
  MultiChannelResampler(
      double srcRate,
      double dstRate,
      int numChannels,
      int maxInLen = DEFAULT_MAX_IN_LEN)
      : BaseResampler(srcRate, dstRate, numChannels, maxInLen) {}

  template <size_t Alignment>
  int process(
      const audioapi::AlignedAudioBuffer<Alignment> &input,
      int length,
      audioapi::AlignedAudioBuffer<Alignment> &output) {
    const size_t numChannels = input.getNumberOfChannels();
    std::array<const float *, audioapi::MAX_CHANNEL_COUNT> inputPtrs{};
    std::array<float *, audioapi::MAX_CHANNEL_COUNT> outputPtrs{};
    for (size_t i = 0; i < numChannels; ++i) {
      inputPtrs[i] = input.getChannel(i)->begin();
      outputPtrs[i] = output.getChannel(i)->begin();
    }
    return BaseResampler::process(inputPtrs.data(), length, outputPtrs.data());
  }
};

class SingleChannelResampler : public BaseResampler {
 public:
  SingleChannelResampler(double srcRate, double dstRate, int maxInLen = DEFAULT_MAX_IN_LEN)
      : BaseResampler(srcRate, dstRate, 1, maxInLen) {}

  template <size_t Alignment>
  int process(
      const audioapi::AlignedAudioArray<Alignment> &input,
      int length,
      audioapi::AlignedAudioArray<Alignment> &output) {
    std::array<const float *, 1> inputPtrs{input.begin()};
    std::array<float *, 1> outputPtrs{output.begin()};
    return BaseResampler::process(inputPtrs.data(), length, outputPtrs.data());
  }
};

} // namespace r8b
