#pragma once

#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/dsp/Resampler.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>

namespace audioapi {

class AudioBus;
class AudioArray;

class WaveShaper {
 public:
  explicit WaveShaper(const std::shared_ptr<AudioArray> &curve);

  void process(const std::shared_ptr<AudioArray> &channelData, int framesToProcess);

  void setCurve(const std::shared_ptr<AudioArray> &curve);
  void setOversample(OverSampleType type);

 private:
  OverSampleType oversample_ = OverSampleType::OVERSAMPLE_NONE;
  std::shared_ptr<AudioArray> curve_;

  // stage 1 Filters (1x <-> 2x)
  std::unique_ptr<Resampler> upSampler_;
  std::unique_ptr<Resampler> downSampler_;

  // stage 2 Filters (2x <-> 4x)
  std::unique_ptr<Resampler> upSampler2_;
  std::unique_ptr<Resampler> downSampler2_;

  std::shared_ptr<AudioArray> tempBuffer2x_;
  std::shared_ptr<AudioArray> tempBuffer4x_;

  void processNone(const std::shared_ptr<AudioArray> &channelData, int framesToProcess);
  void process2x(const std::shared_ptr<AudioArray> &channelData, int framesToProcess);
  void process4x(const std::shared_ptr<AudioArray> &channelData, int framesToProcess);
};

} // namespace audioapi
