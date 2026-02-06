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

class AudioBuffer;
class AudioArray;
class CircularAudioArray;
struct AnalyserOptions;

class AnalyserNode : public AudioNode {
 public:
  enum class WindowType { BLACKMAN, HANN };
  explicit AnalyserNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AnalyserOptions &options);

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
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  int fftSize_;
  float minDecibels_;
  float maxDecibels_;
  float smoothingTimeConstant_;

  WindowType windowType_;
  std::shared_ptr<AudioArray> windowData_;

  std::unique_ptr<CircularAudioArray> inputArray_;
  std::unique_ptr<AudioBuffer> downMixBuffer_;
  std::unique_ptr<AudioArray> tempArray_;

  std::unique_ptr<dsp::FFT> fft_;
  std::vector<std::complex<float>> complexData_;
  std::unique_ptr<AudioArray> magnitudeArray_;
  bool shouldDoFFTAnalysis_{true};

  void doFFTAnalysis();

  void setWindowData(WindowType type, int size);
};

} // namespace audioapi
