#pragma once

#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <cstddef>
#include <memory>
#include <vector>

// Sample rate converter designed by Aleksey Vaneev of Voxengo on MIT license
#include "CDSPResampler.h"

namespace r8b {

class BaseResampler {
  BaseResampler(const BaseResampler &) = delete;
  BaseResampler &operator=(const BaseResampler &) = delete;
  BaseResampler(BaseResampler &&) noexcept = default;
  BaseResampler &operator=(BaseResampler &&) noexcept = default;

 private:
  std::vector<std::unique_ptr<CDSPResampler24>> resamplers_;
  std::vector<std::vector<double>> inputBuffers_;

 protected:
  inline BaseResampler(double srcRate, double dstRate, int numChannels, int maxInLen = 2048) {
    resamplers_.reserve(static_cast<size_t>(numChannels));
    inputBuffers_.reserve(static_cast<size_t>(numChannels));
    for (int i = 0; i < numChannels; ++i) {
      resamplers_.emplace_back(std::make_unique<CDSPResampler24>(srcRate, dstRate, maxInLen));
      inputBuffers_.emplace_back(static_cast<size_t>(maxInLen));
    }
  }

  inline int process(const std::vector<float *> &input, int l, std::vector<float *> &output) {
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

 public:
  virtual ~BaseResampler() = default;

  /** @return Maximum number of output samples per channel for one process() call (for the maxInLen passed to the constructor). */
  inline int getMaxOutLen() const {
    if (resamplers_.empty()) {
      return 0;
    }
    return resamplers_[0]->getMaxOutLen(0);
  }
};

class MultiChannelResampler : public BaseResampler {
 public:
  inline MultiChannelResampler(double srcRate, double dstRate, int numChannels, int maxInLen = 2048)
      : BaseResampler(srcRate, dstRate, numChannels, maxInLen) {}

  ~MultiChannelResampler() = default;

  template <size_t Alignment>
  int process(
      const audioapi::AlignedAudioBuffer<Alignment> &input,
      int length,
      audioapi::AlignedAudioBuffer<Alignment> &output) {
    const size_t numChannels = input.getNumberOfChannels();
    std::vector<float *> inputPtrs(numChannels);
    std::vector<float *> outputPtrs(numChannels);
    for (size_t i = 0; i < numChannels; ++i) {
      inputPtrs[i] = input.getChannel(i)->begin();
      outputPtrs[i] = output.getChannel(i)->begin();
    }
    return BaseResampler::process(inputPtrs, length, outputPtrs);
  }
};

class SingleChannelResampler : public BaseResampler {
 public:
  inline SingleChannelResampler(double srcRate, double dstRate, int maxInLen = 2048)
      : BaseResampler(srcRate, dstRate, 1, maxInLen) {}

  ~SingleChannelResampler() = default;

  template <size_t Alignment>
  int process(
      const audioapi::AlignedAudioArray<Alignment> &input,
      int length,
      audioapi::AlignedAudioArray<Alignment> &output) {
    std::vector<float *> inputPtrs(1);
    std::vector<float *> outputPtrs(1);
    inputPtrs[0] = const_cast<float *>(input.begin());
    outputPtrs[0] = output.begin();
    return BaseResampler::process(inputPtrs, length, outputPtrs);
  }
};

} // namespace r8b
