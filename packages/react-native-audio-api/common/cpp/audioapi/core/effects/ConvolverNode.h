#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/dsp/Convolver.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>
#include <vector>

#include <audioapi/utils/ThreadPool.hpp>

static constexpr int GAIN_CALIBRATION =
    -58; // magic number so that processed signal and dry signal have roughly the same volume
static constexpr float MIN_IR_POWER = 0.000125;

namespace audioapi {

struct ConvolverOptions;

class ConvolverNode : public AudioNode {
 public:
  explicit ConvolverNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const ConvolverOptions &options);

  /// @note Audio Thread only
  void setBuffer(
      const std::shared_ptr<AudioBuffer> &buffer,
      std::vector<std::unique_ptr<Convolver>> convolvers,
      const std::shared_ptr<ThreadPool> &threadPool,
      const std::shared_ptr<DSPAudioBuffer> &internalBuffer,
      const std::shared_ptr<DSPAudioBuffer> &intermediateBuffer,
      float scaleFactor);

  float calculateNormalizationScale(const std::shared_ptr<AudioBuffer> &buffer) const;

 protected:
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  void onInputDisabled() override;
  const float gainCalibrationSampleRate_;
  size_t remainingSegments_;
  size_t internalBufferIndex_;
  bool signalledToStop_;
  float scaleFactor_;
  std::shared_ptr<DSPAudioBuffer> intermediateBuffer_;

  // impulse response buffer
  std::shared_ptr<AudioBuffer> buffer_;
  // buffer to hold internal processed data
  std::shared_ptr<DSPAudioBuffer> internalBuffer_;
  // vectors of convolvers, one per channel
  std::vector<std::unique_ptr<Convolver>> convolvers_;
  std::shared_ptr<ThreadPool> threadPool_;

  void performConvolution(const std::shared_ptr<DSPAudioBuffer> &processingBuffer);
};

} // namespace audioapi
