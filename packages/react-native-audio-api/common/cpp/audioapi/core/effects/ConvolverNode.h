#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/dsp/Convolver.h>

#include <memory>
#include <vector>

#include <audioapi/utils/ThreadPool.hpp>

static constexpr int GAIN_CALIBRATION = -58; // magic number so that processed signal and dry signal have roughly the same volume
static constexpr double MIN_IR_POWER = 0.000125;

namespace audioapi {

class AudioBus;
class AudioBuffer;

class ConvolverNode : public AudioNode {
 public:
    explicit ConvolverNode(BaseAudioContext *context, const std::shared_ptr<AudioBuffer>& buffer, bool disableNormalization);

    [[nodiscard]] bool getNormalize_() const;
    [[nodiscard]] const std::shared_ptr<AudioBuffer> &getBuffer() const;
    void setNormalize(bool normalize);
    void setBuffer(const std::shared_ptr<AudioBuffer> &buffer);

 protected:
  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  std::shared_ptr<AudioBus> processInputs(const std::shared_ptr<AudioBus>& outputBus, int framesToProcess, bool checkIsAlreadyProcessed) override;
  void onInputDisabled() override;
  float gainCalibrationSampleRate_;
  size_t remainingSegments_;
  size_t internalBufferIndex_;
  bool normalize_;
  bool signalledToStop_;
  float scaleFactor_;
  std::shared_ptr<AudioBus>intermediateBus_;

  // impulse response buffer
  std::shared_ptr<AudioBuffer> buffer_;
  // buffer to hold internal processed data
  std::shared_ptr<AudioBus> internalBuffer_;
  // vectors of convolvers, one per channel
  std::vector<Convolver> convolvers_;
  std::shared_ptr<ThreadPool> threadPool_;

  void calculateNormalizationScale();
  void performConvolution(const std::shared_ptr<AudioBus>& processingBus);
};

} // namespace audioapi
