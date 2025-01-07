#pragma once

#include <memory>

#include "AudioNode.h"

namespace audioapi {

class AudioBus;
class AudioArray;

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

  void getFloatFrequencyData(float *data);
  void getByteFrequencyData(float *data);
  void getFloatTimeDomainData(float *data);
  void getByteTimeDomainData(float *data);

 protected:
  void processNode(AudioBus *processingBus, int framesToProcess) override;

 private:
  int fftSize_;
  double minDecibels_;
  double maxDecibels_;
  double smoothingTimeConstant_;

  std::unique_ptr<AudioArray> inputBuffer_;
  std::unique_ptr<AudioBus> downMixBus_;
  int vWriteIndex_;
};

} // namespace audioapi
