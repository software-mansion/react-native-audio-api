#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/dsp/Resampler.h>
#include <audioapi/dsp/WaveShaper.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace audioapi {

class AudioBuffer;
class AudioArrayBuffer;
struct WaveShaperOptions;

class WaveShaperNode : public AudioNode {
 public:
  explicit WaveShaperNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const WaveShaperOptions &options);

  [[nodiscard]] OverSampleType getOversample() const;
  [[nodiscard]] std::shared_ptr<AudioArrayBuffer> getCurve() const;

  void setOversample(OverSampleType);
  void setCurve(const std::shared_ptr<AudioArrayBuffer> &curve);

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  std::atomic<OverSampleType> oversample_;
  std::shared_ptr<AudioArrayBuffer> curve_;
  mutable std::mutex mutex_;

  std::vector<std::unique_ptr<WaveShaper>> waveShapers_{};
};

} // namespace audioapi
