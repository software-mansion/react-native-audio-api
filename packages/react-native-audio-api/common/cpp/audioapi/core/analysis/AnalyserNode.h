#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/dsp/FFT.h>
#include <audioapi/dsp/Windows.hpp>

#include <complex>
#include <cstddef>
#include <memory>
#include <vector>

namespace audioapi {

class AudioBuffer;
class AudioArray;
class CircularAudioArray;
struct AnalyserOptions;

class AnalyserNode : public AudioNode {
 public:
  explicit AnalyserNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AnalyserOptions &options);

  void setFFTSize(
      int fftSize,
      const std::shared_ptr<dsp::FFT> &fft,
      const std::vector<std::complex<float>> &complexData,
      const std::shared_ptr<AudioArray> &magnitudeArray,
      const std::shared_ptr<AudioArray> &tempArray,
      const std::shared_ptr<AudioArray> &windowData);
  void setMinDecibels(float minDecibels);
  void setMaxDecibels(float maxDecibels);
  void setSmoothingTimeConstant(float smoothingTimeConstant);

  void getFloatFrequencyData(float *data, int length);
  void getByteFrequencyData(uint8_t *data, int length);
  void getFloatTimeDomainData(float *data, int length);
  void getByteTimeDomainData(uint8_t *data, int length);

  static inline std::shared_ptr<AudioArray> createWindowData(int fftSize) {
    auto windowData = std::make_shared<AudioArray>(fftSize);
    dsp::Blackman().apply(windowData->span());
    return windowData;
  }

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  int fftSize_;
  float minDecibels_;
  float maxDecibels_;
  float smoothingTimeConstant_;

  std::unique_ptr<CircularAudioArray> inputArray_;
  std::unique_ptr<AudioBuffer> downMixBuffer_;

  std::shared_ptr<dsp::FFT> fft_;
  std::shared_ptr<AudioArray> tempArray_;
  std::shared_ptr<AudioArray> windowData_;
  std::vector<std::complex<float>> complexData_;
  std::shared_ptr<AudioArray> magnitudeArray_;
  bool shouldDoFFTAnalysis_{true};

  void doFFTAnalysis();
};

} // namespace audioapi
