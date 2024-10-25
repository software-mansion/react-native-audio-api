#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

class BiquadFilterNode : public AudioNode {
 public:
  explicit BiquadFilterNode(AudioContext *context);

  std::string getType() const;
  void setType(const std::string &type);
  std::shared_ptr<AudioParam> getFrequencyParam() const;
  std::shared_ptr<AudioParam> getDetuneParam() const;
  std::shared_ptr<AudioParam> getQParam() const;
  std::shared_ptr<AudioParam> getGainParam() const;
  void getFrequencyResponse(
      const std::vector<float> &frequencyArray,
      std::vector<float> &magResponseOutput,
      std::vector<float> &phaseResponseOutput);

 protected:
  bool processAudio(float *audioData, int32_t numFrames) override;

 private:
  enum class FilterType {
    LOWPASS,
    HIGHPASS,
    BANDPASS,
    LOWSHELF,
    HIGHSHELF,
    PEAKING,
    NOTCH,
    ALLPASS
  };

  static FilterType fromString(const std::string &type) {
    std::string lowerType = type;
    std::transform(
        lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

    if (lowerType == "lowpass")
      return FilterType::LOWPASS;
    if (lowerType == "highpass")
      return FilterType::HIGHPASS;
    if (lowerType == "bandpass")
      return FilterType::BANDPASS;
    if (lowerType == "lowshelf")
      return FilterType::LOWSHELF;
    if (lowerType == "highshelf")
      return FilterType::HIGHSHELF;
    if (lowerType == "peaking")
      return FilterType::PEAKING;
    if (lowerType == "notch")
      return FilterType::NOTCH;
    if (lowerType == "allpass")
      return FilterType::ALLPASS;

    throw std::invalid_argument("Invalid filter type: " + type);
  }

  static std::string toString(FilterType type) {
    switch (type) {
      case FilterType::LOWPASS:
        return "lowpass";
      case FilterType::HIGHPASS:
        return "highpass";
      case FilterType::BANDPASS:
        return "bandpass";
      case FilterType::LOWSHELF:
        return "lowshelf";
      case FilterType::HIGHSHELF:
        return "highshelf";
      case FilterType::PEAKING:
        return "peaking";
      case FilterType::NOTCH:
        return "notch";
      case FilterType::ALLPASS:
        return "allpass";
      default:
        throw std::invalid_argument("Unknown filter type");
    }
  }

 private:
  std::shared_ptr<AudioParam> frequencyParam_;
  std::shared_ptr<AudioParam> detuneParam_;
  std::shared_ptr<AudioParam> QParam_;
  std::shared_ptr<AudioParam> gainParam_;
  FilterType type_;

  // delayed samples
  float x1_ = 0;
  float x2_ = 0;
  float y1_ = 0;
  float y2_ = 0;

  // coefficients
  float b0_ = 1.0;
  float b1_ = 0;
  float b2_ = 0;
  float a1_ = 1.0;
  float a2_ = 0;

  static float clamp(float value, float min, float max);
  void resetCoefficients();
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
