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

  size_t getFftSize() const;
  size_t getFrequencyBinCount() const;
  float getMinDecibels() const;
  float getMaxDecibels() const;

  float getSmoothingTimeConstant() const;
  void setFftSize(size_t fftSize);
  void setMinDecibels(float minDecibels);
  void setMaxDecibels(float maxDecibels);
  void setSmoothingTimeConstant(float smoothingTimeConstant);

  void getFloatFrequencyData(float *data, size_t length);
  void getByteFrequencyData(uint8_t *data, size_t length);
  void getFloatTimeDomainData(float *data, size_t length);
  void getByteTimeDomainData(uint8_t *data, size_t length);

 protected:
  void processNode(AudioBus *processingBus, int framesToProcess) override;

 private:
  size_t fftSize_;
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
  static void applyWindow(float *data, size_t length);
};

} // namespace audioapi
