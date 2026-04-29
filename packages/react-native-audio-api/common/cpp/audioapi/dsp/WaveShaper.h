#pragma once

#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/dsp/r8brain/Resampler.hpp>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>
#include <utility>

namespace audioapi {

using SingleChannelResamplerPtr = std::unique_ptr<r8b::SingleChannelResampler>;

class WaveShaper {
 public:
  explicit WaveShaper(const std::shared_ptr<AudioArray> &curve, float sampleRate);

  void process(DSPAudioArray &channelData, int framesToProcess);

  void setCurve(const std::shared_ptr<AudioArray> &curve);

  /// @note Audio thread only. Resamplers must be pre-built on the JS thread
  /// via `makeResamplers`. This method only swaps pointers and disposes the
  /// previous resamplers off-thread.
  void setOversample(
      OverSampleType type,
      SingleChannelResamplerPtr upSampler,
      SingleChannelResamplerPtr downSampler,
      utils::Disposer<DISPOSER_PAYLOAD_SIZE> &disposer);

  /// @note JS thread (or any non-audio thread). Builds a matched up/down
  /// resampler pair for the given oversample type. Returns {nullptr, nullptr}
  /// for OVERSAMPLE_NONE.
  static std::pair<SingleChannelResamplerPtr, SingleChannelResamplerPtr> makeResamplers(
      OverSampleType type,
      float sampleRate);

 private:
  OverSampleType oversample_ = OverSampleType::OVERSAMPLE_NONE;
  std::shared_ptr<AudioArray> curve_;
  float sampleRate_;

  SingleChannelResamplerPtr upSampler_;
  SingleChannelResamplerPtr downSampler_;

  std::shared_ptr<DSPAudioArray> tempBuffer2x_;
  std::shared_ptr<DSPAudioArray> tempBuffer4x_;

  void processNone(DSPAudioArray &channelData, int framesToProcess);
  void processResampled(DSPAudioArray &channelData, int framesToProcess);
};

} // namespace audioapi
