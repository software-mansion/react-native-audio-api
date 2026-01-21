#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/dsp/FFT.h>

#include <algorithm>
#include <complex>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class AudioBus;
class AudioArray;
class CircularAudioArray;
class AnalyserOptions;

class AnalyserNode : public AudioNode {
 public:
  enum class WindowType { BLACKMAN, HANN };
  explicit AnalyserNode(std::shared_ptr<BaseAudioContext> context, const AnalyserOptions &options);

  int getFftSize() const;
  int getFrequencyBinCount() const;
  float getMinDecibels() const;
  float getMaxDecibels() const;
  float getSmoothingTimeConstant() const;
  AnalyserNode::WindowType getWindowType() const;

  void setFftSize(int fftSize);
  void setMinDecibels(float minDecibels);
  void setMaxDecibels(float maxDecibels);
  void setSmoothingTimeConstant(float smoothingTimeConstant);
  void setWindowType(AnalyserNode::WindowType);

  void getFloatFrequencyData(float *data, int length);
  void getByteFrequencyData(uint8_t *data, int length);
  void getFloatTimeDomainData(float *data, int length);
  void getByteTimeDomainData(uint8_t *data, int length);

 protected:
  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;

 private:
  int fftSize_;
  float minDecibels_;
  float maxDecibels_;
  float smoothingTimeConstant_;

  WindowType windowType_;
  std::shared_ptr<AudioArray> windowData_;

  std::unique_ptr<CircularAudioArray> inputBuffer_;
  std::unique_ptr<AudioBus> downMixBus_;
  std::unique_ptr<AudioArray> tempBuffer_;

  std::unique_ptr<dsp::FFT> fft_;
  std::vector<std::complex<float>> complexData_;
  std::unique_ptr<AudioArray> magnitudeBuffer_;
  bool shouldDoFFTAnalysis_{true};

  void doFFTAnalysis();

  void setWindowData(WindowType type, int size);
};

} // namespace audioapi
