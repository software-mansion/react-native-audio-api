#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/dsp/Convolver.h>

#include <memory>
#include <vector>

namespace audioapi {

class AudioBus;
class AudioBuffer;

class ConvolverNode : public AudioNode {
 public:
    explicit ConvolverNode(BaseAudioContext *context, std::shared_ptr<AudioBuffer> buffer, bool disableNormalization);

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
  size_t remainingSegments_ = 0;
  bool normalize_ = true;
  bool signaledToStop_ = false;
  int internalBufferIndex_ = 0;
  float scaleFactor_ = 1.0f;
  float gainCalibration_ = -62; //magic number so that processed signal and dry signal have roughly the same volume
  float minPower_ = 0.000125;
  float thirdChannelData_[RENDER_QUANTUM_SIZE];
  float fourthChannelData_[RENDER_QUANTUM_SIZE];
  std::shared_ptr<AudioBuffer> buffer_;
  std::shared_ptr<AudioBus> internalBuffer_;
  std::vector<Convolver> convolvers_;
  void calculateNormalizationScale();
  void performConvolution(const std::shared_ptr<AudioBus>& processingBus);
};

} // namespace audioapi
