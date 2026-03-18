#pragma once

#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/dsp/r8brain/Resampler.hpp>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>

namespace audioapi {

class WaveShaper {
 public:
  explicit WaveShaper(const std::shared_ptr<AudioArray> &curve, float sampleRate);

  void process(DSPAudioArray &channelData, int framesToProcess);

  void setCurve(const std::shared_ptr<AudioArray> &curve);
  void setOversample(OverSampleType type);

 private:
  OverSampleType oversample_ = OverSampleType::OVERSAMPLE_NONE;
  std::shared_ptr<AudioArray> curve_;
  float sampleRate_;

  std::unique_ptr<r8b::SingleChannelResampler> upSampler_;
  std::unique_ptr<r8b::SingleChannelResampler> downSampler_;

  std::shared_ptr<DSPAudioArray> tempBuffer2x_;
  std::shared_ptr<DSPAudioArray> tempBuffer4x_;

  void createResamplers(OverSampleType type);
  void processNone(DSPAudioArray &channelData, int framesToProcess);
  void processResampled(DSPAudioArray &channelData, int framesToProcess);
};

} // namespace audioapi
