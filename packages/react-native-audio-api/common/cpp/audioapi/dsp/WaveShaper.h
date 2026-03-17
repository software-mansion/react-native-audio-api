#pragma once

#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/dsp/r8brain/Resampler.h>

#include <memory>

namespace audioapi {

class AudioBuffer;
class AudioArray;

class WaveShaper {
 public:
  explicit WaveShaper(const std::shared_ptr<AudioArray> &curve, float sampleRate);

  void process(AudioArray &channelData, int framesToProcess);

  void setCurve(const std::shared_ptr<AudioArray> &curve);
  void setOversample(OverSampleType type);

 private:
  OverSampleType oversample_ = OverSampleType::OVERSAMPLE_NONE;
  std::shared_ptr<AudioArray> curve_;
  float sampleRate_;

  std::unique_ptr<r8b::SingleChannelResampler> upSampler_;
  std::unique_ptr<r8b::SingleChannelResampler> downSampler_;

  std::shared_ptr<AudioArray> tempBuffer2x_;
  std::shared_ptr<AudioArray> tempBuffer4x_;

  void createResamplers(OverSampleType type);
  void processNone(AudioArray &channelData, int framesToProcess);
  void processResampled(AudioArray &channelData, int framesToProcess);
};

} // namespace audioapi
