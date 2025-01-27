#pragma once

#include <memory>

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
  double getMinDecibels() const;
  double getMaxDecibels() const;

  double getSmoothingTimeConstant() const;
  void setFftSize(size_t fftSize);
  void setMinDecibels(double minDecibels);
  void setMaxDecibels(double maxDecibels);
  void setSmoothingTimeConstant(double smoothingTimeConstant);

  void getFloatFrequencyData(float *data, size_t length);
  void getByteFrequencyData(uint8_t *data, size_t length);
  void getFloatTimeDomainData(float *data, size_t length);
  void getByteTimeDomainData(uint8_t *data, size_t length);

 protected:
  void processNode(AudioBus *processingBus, int framesToProcess) override;

 private:
  size_t fftSize_;
  double minDecibels_;
  double maxDecibels_;
  double smoothingTimeConstant_;

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
