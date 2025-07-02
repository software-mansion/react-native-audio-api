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
    explicit ConvolverNode(BaseAudioContext *context);

    [[nodiscard]] bool getNormalize_() const;
    [[nodiscard]] const std::shared_ptr<AudioBuffer> &getBuffer() const;
    void setNormalize(bool normalize);
    void setBuffer(const std::shared_ptr<AudioBuffer> &buffer);

 protected:
    void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  bool normalize_ = true;
  std::shared_ptr<AudioBuffer> buffer_;
  void calculateNormalizationScale();
  float scaleFactor_ = 1.0f;
  float GainCalibration = 0.00125;
  float GainCalibrationSampleRate = 44100.0f;
  float MinPower = 0.000125;
  std::shared_ptr<Convolver> convolver_;
};

} // namespace audioapi
