#pragma once

#include <memory>

#include "AudioNode.h"

namespace audioapi {

class AudioBus;

class AnalyserNode : public AudioNode {
 public:
  explicit AnalyserNode(BaseAudioContext *context);

  int getFftSize() const;
  int getFrequencyBinCount() const;
  double getMinDecibels() const;
  double getMaxDecibels() const;

  double getSmoothingTimeConstant() const;
  void setFftSize(int fftSize);
  void setMinDecibels(double minDecibels);
  void setMaxDecibels(double maxDecibels);
  void setSmoothingTimeConstant(double smoothingTimeConstant);

  float *getFloatFrequencyData();
  uint8_t *getByteFrequencyData();
  float *getFloatTimeDomainData();
  uint8_t *getByteTimeDomainData();

 protected:
  void processNode(AudioBus *processingBus, int framesToProcess) override;

 private:
  int fftSize_;
  double minDecibels_;
  double maxDecibels_;
  double smoothingTimeConstant_;

  std::unique_ptr<AudioBus> inputBus_;
};

} // namespace audioapi
