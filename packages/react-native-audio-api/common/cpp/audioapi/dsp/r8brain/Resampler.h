#pragma once

#include <audioapi/utils/AudioBuffer.h>
#include <cstddef>
#include <memory>
#include <vector>

// Sample rate converter designed by Aleksey Vaneev of Voxengo on MIT license
#include "CDSPResampler.h"

namespace r8b {

class MultiChannelResampler {
 public:
  MultiChannelResampler(double srcRate, double dstRate, int numChannels, int maxInLen = 2048);
  ~MultiChannelResampler() = default;
  MultiChannelResampler(const MultiChannelResampler &) = delete;
  MultiChannelResampler &operator=(const MultiChannelResampler &) = delete;
  MultiChannelResampler(MultiChannelResampler &&) noexcept = default;
  MultiChannelResampler &operator=(MultiChannelResampler &&) noexcept = default;

  int process(const std::vector<float *> &input, int l, std::vector<float *> &output);
  int process(const audioapi::AudioBuffer &input, int l, audioapi::AudioBuffer &output);

 private:
  std::vector<std::unique_ptr<CDSPResampler24>> resamplers_;
  std::vector<std::vector<double>> inputBuffers_;
  std::vector<std::vector<float>> outputBuffers_;
};
} // namespace r8b
