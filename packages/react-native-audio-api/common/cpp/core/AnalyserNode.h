#pragma once

#include <memory>
#include <cstddef>

#include "AudioNode.h"

namespace audioapi {

class AudioBus;
class AudioArray;
class FFTFrame;

class AnalyserNode : public AudioNode {
 public:
  explicit AnalyserNode(BaseAudioContext *context);

  int getFftSize() const;
  int getFrequencyBinCount() const;
  float getMinDecibels() const;
  float getMaxDecibels() const;

  float getSmoothingTimeConstant() const;
  void setFftSize(int fftSize);
  void setMinDecibels(float minDecibels);
  void setMaxDecibels(float maxDecibels);
  void setSmoothingTimeConstant(float smoothingTimeConstant);

  void getFloatFrequencyData(float *data, int length);
  void getByteFrequencyData(uint8_t *data, int length);
  void getFloatTimeDomainData(float *data, int length);
  void getByteTimeDomainData(uint8_t *data, int length);

 protected:
  void processNode(AudioBus *processingBus, int framesToProcess) override;

 private:
  int fftSize_;
  float minDecibels_;
  float maxDecibels_;
  float smoothingTimeConstant_;

  std::unique_ptr<AudioArray> inputBuffer_;
  std::unique_ptr<AudioBus> downMixBus_;
  int vWriteIndex_;

  std::unique_ptr<FFTFrame> fftFrame_;
  std::unique_ptr<AudioArray> magnitudeBuffer_;
  bool shouldDoFFTAnalysis_ { true };

  void doFFTAnalysis();
  static void applyWindow(float *data, int length);
};

} // namespace audioapi
