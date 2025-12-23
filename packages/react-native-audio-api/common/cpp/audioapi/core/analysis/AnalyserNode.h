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

class AnalyserNode : public AudioNode {
 public:
  enum class WindowType { BLACKMAN, HANN };
  explicit AnalyserNode(std::shared_ptr<BaseAudioContext> context);

  int getFftSize() const;
  int getFrequencyBinCount() const;
  float getMinDecibels() const;
  float getMaxDecibels() const;
  float getSmoothingTimeConstant() const;
  std::string getWindowType() const;

  void setFftSize(int fftSize);
  void setMinDecibels(float minDecibels);
  void setMaxDecibels(float maxDecibels);
  void setSmoothingTimeConstant(float smoothingTimeConstant);
  void setWindowType(const std::string &type);

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

  static WindowType fromString(const std::string &type) {
    std::string lowerType = type;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
    if (lowerType == "blackman") {
      return WindowType::BLACKMAN;
    }
    if (lowerType == "hann") {
      return WindowType::HANN;
    }

    throw std::invalid_argument("Unknown window type");
  }

  static std::string toString(WindowType type) {
    switch (type) {
      case WindowType::BLACKMAN:
        return "blackman";
      case WindowType::HANN:
        return "hann";
      default:
        throw std::invalid_argument("Unknown window type");
    }
  }

  void doFFTAnalysis();

  void setWindowData(WindowType type, int size);
};

} // namespace audioapi
