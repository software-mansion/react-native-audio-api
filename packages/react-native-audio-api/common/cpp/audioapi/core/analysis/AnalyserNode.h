#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/dsp/FFTFrame.h>

#include <memory>
#include <cstddef>
#include <string>
#include <complex>

namespace audioapi {

class AudioBus;
class AudioArray;

class AnalyserNode : public AudioNode {
 public:
  enum class WindowType { BLACKMAN, HANN };
  explicit AnalyserNode(BaseAudioContext *context);

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
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  int fftSize_;
  float minDecibels_;
  float maxDecibels_;
  float smoothingTimeConstant_;
  WindowType windowType_;

  std::unique_ptr<AudioArray> inputBuffer_;
  std::unique_ptr<AudioBus> downMixBus_;
  int vWriteIndex_;

  std::unique_ptr<FFTFrame> fftFrame_;
  std::shared_ptr<AudioArray> realData_;
  std::shared_ptr<AudioArray> imaginaryData_;
  std::unique_ptr<AudioArray> magnitudeBuffer_;
  bool shouldDoFFTAnalysis_ { true };

  static WindowType fromString(const std::string &type) {
    std::string lowerType = type;
    std::transform(
        lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
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
  static void applyBlackManWindow(float *data, int length);
  static void applyHannWindow(float *data, int length);
};

} // namespace audioapi
