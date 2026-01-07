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

class AudioBus;
class AudioArray;

class WaveShaperNode : public AudioNode {
 public:
  explicit WaveShaperNode(std::shared_ptr<BaseAudioContext> context);

  [[nodiscard]] std::string getOversample() const;
  [[nodiscard]] std::shared_ptr<AudioArray> getCurve() const;

  void setOversample(const std::string &type);
  void setCurve(const std::shared_ptr<AudioArray> &curve);

 protected:
  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;

 private:
  std::atomic<OverSampleType> oversample_;
  std::shared_ptr<AudioArray> curve_{};
  mutable std::mutex mutex_;

  std::vector<std::unique_ptr<WaveShaper>> waveShapers_{};

  static OverSampleType overSampleTypeFromString(const std::string &type) {
    std::string lowerType = type;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

    if (lowerType == "2x")
      return OverSampleType::OVERSAMPLE_2X;
    if (lowerType == "4x")
      return OverSampleType::OVERSAMPLE_4X;

    return OverSampleType::OVERSAMPLE_NONE;
  }

  static std::string overSampleTypeToString(OverSampleType type) {
    switch (type) {
      case OverSampleType::OVERSAMPLE_2X:
        return "2x";
      case OverSampleType::OVERSAMPLE_4X:
        return "4x";
      default:
        return "none";
    }
  }
};

} // namespace audioapi
