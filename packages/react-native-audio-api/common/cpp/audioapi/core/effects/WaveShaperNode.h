#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/dsp/WaveShaper.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

struct WaveShaperOptions;

/// @brief Pre-built oversample state shipped from the JS thread to the audio
/// thread. Heap-allocated once on the JS thread; the audio event lambda only
/// captures the 8-byte `unique_ptr<OversampleUpdate>`, which fits within the
/// disposer payload and the FatFunction inline storage.
struct OversampleUpdate {
  OverSampleType type = OverSampleType::OVERSAMPLE_NONE;
  std::vector<std::pair<SingleChannelResamplerPtr, SingleChannelResamplerPtr>> pairs;
};

class WaveShaperNode : public AudioNode {
 public:
  explicit WaveShaperNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const WaveShaperOptions &options);

  /// @note Audio Thread only. Takes ownership of pre-built resamplers and
  /// disposes any previous ones via the supplied disposer.
  void setOversample(
      std::unique_ptr<OversampleUpdate> update,
      utils::Disposer<DISPOSER_PAYLOAD_SIZE> &disposer);

  /// @note Audio Thread only
  void setCurve(const std::shared_ptr<AudioArray> &curve);

 protected:
  void processNode(int framesToProcess) override;

 private:
  OverSampleType oversample_;
  std::shared_ptr<AudioArray> curve_;

  std::vector<std::unique_ptr<WaveShaper>> waveShapers_;
};

} // namespace audioapi
