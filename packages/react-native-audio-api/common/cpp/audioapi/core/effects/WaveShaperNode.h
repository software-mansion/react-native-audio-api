#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/dsp/WaveShaper.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>
#include <vector>

namespace audioapi {

struct WaveShaperOptions;

class WaveShaperNode : public AudioNode {
 public:
  explicit WaveShaperNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const WaveShaperOptions &options);

  /// @note Audio Thread only
  void setOversample(OverSampleType);

  /// @note Audio Thread only
  void setCurve(const std::shared_ptr<AudioArray> &curve);

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  OverSampleType oversample_;
  std::shared_ptr<AudioArray> curve_;

  std::vector<std::unique_ptr<WaveShaper>> waveShapers_{};
};

} // namespace audioapi
