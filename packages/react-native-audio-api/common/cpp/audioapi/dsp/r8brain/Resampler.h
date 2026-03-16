#pragma once

#include <audioapi/core/utils/Macros.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
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
  static constexpr int DEFAULT_MAX_IN_LEN = 2048;

 protected:
  BaseResampler(double srcRate, double dstRate, int numChannels, int maxInLen = DEFAULT_MAX_IN_LEN);
  int process(std::vector<float *> &input, int length, std::vector<float *> &output);

 public:
  virtual ~BaseResampler() = default;
  /** @return Maximum number of output samples per channel for one process() call (for the maxInLen passed to the constructor). */
  int getMaxOutLen() const;
};

class MultiChannelResampler : public BaseResampler {
 public:
  MultiChannelResampler(double srcRate, double dstRate, int numChannels, int maxInLen);
  ~MultiChannelResampler() = default;
  DELETE_COPY_AND_MOVE(MultiChannelResampler);
  int process(audioapi::AudioBuffer &input, int length, audioapi::AudioBuffer &output);
};

class SingleChannelResampler : public BaseResampler {
 public:
  SingleChannelResampler(double srcRate, double dstRate, int maxInLen);
  ~SingleChannelResampler() = default;
  DELETE_COPY_AND_MOVE(SingleChannelResampler);
  int process(audioapi::AudioArray &input, int length, audioapi::AudioArray &output);
};
} // namespace r8b
