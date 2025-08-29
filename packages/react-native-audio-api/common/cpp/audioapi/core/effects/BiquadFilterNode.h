#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/types/BiquadFilterType.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace audioapi {

class AudioBus;

class BiquadFilterNode : public AudioNode {
 public:
  explicit BiquadFilterNode(BaseAudioContext *context);

  [[nodiscard]] std::string getType();
  void setType(const std::string &type);
  [[nodiscard]] std::shared_ptr<AudioParam> getFrequencyParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getQParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getGainParam() const;
  void getFrequencyResponse(
      const float *frequencyArray,
      float *magResponseOutput,
      float *phaseResponseOutput,
      int length);

 protected:
  void processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> frequencyParam_;
  std::shared_ptr<AudioParam> detuneParam_;
  std::shared_ptr<AudioParam> QParam_;
  std::shared_ptr<AudioParam> gainParam_;
  audioapi::BiquadFilterType type_;

  // delayed samples
  float x1_ = 0;
  float x2_ = 0;
  float y1_ = 0;
  float y2_ = 0;

  // coefficients
  float b0_ = 1.0;
  float b1_ = 0;
  float b2_ = 0;
  float a1_ = 0;
  float a2_ = 0;

  static BiquadFilterType fromString(const std::string &type) {
    std::string lowerType = type;
    std::transform(
        lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

    if (lowerType == "lowpass")
      return BiquadFilterType::LOWPASS;
    if (lowerType == "highpass")
      return BiquadFilterType::HIGHPASS;
    if (lowerType == "bandpass")
      return BiquadFilterType::BANDPASS;
    if (lowerType == "lowshelf")
      return BiquadFilterType::LOWSHELF;
    if (lowerType == "highshelf")
      return BiquadFilterType::HIGHSHELF;
    if (lowerType == "peaking")
      return BiquadFilterType::PEAKING;
    if (lowerType == "notch")
      return BiquadFilterType::NOTCH;
    if (lowerType == "allpass")
      return BiquadFilterType::ALLPASS;

    throw std::invalid_argument("Invalid filter type: " + type);
  }

  static std::string toString(BiquadFilterType type) {
    switch (type) {
      case BiquadFilterType::LOWPASS:
        return "lowpass";
      case BiquadFilterType::HIGHPASS:
        return "highpass";
      case BiquadFilterType::BANDPASS:
        return "bandpass";
      case BiquadFilterType::LOWSHELF:
        return "lowshelf";
      case BiquadFilterType::HIGHSHELF:
        return "highshelf";
      case BiquadFilterType::PEAKING:
        return "peaking";
      case BiquadFilterType::NOTCH:
        return "notch";
      case BiquadFilterType::ALLPASS:
        return "allpass";
      default:
        throw std::invalid_argument("Unknown filter type");
    }
  }

  void setNormalizedCoefficients(
      float b0,
      float b1,
      float b2,
      float a0,
      float a1,
      float a2);
  void setLowpassCoefficients(float frequency, float Q);
  void setHighpassCoefficients(float frequency, float Q);
  void setBandpassCoefficients(float frequency, float Q);
  void setLowshelfCoefficients(float frequency, float gain);
  void setHighshelfCoefficients(float frequency, float gain);
  void setPeakingCoefficients(float frequency, float Q, float gain);
  void setNotchCoefficients(float frequency, float Q);
  void setAllpassCoefficients(float frequency, float Q);
  void applyFilter();
};

} // namespace audioapi
