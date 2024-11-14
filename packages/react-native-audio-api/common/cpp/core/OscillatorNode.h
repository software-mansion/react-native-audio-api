#pragma once

#include <cmath>
#include <memory>
#include <string>

#include "AudioParam.h"
#include "AudioScheduledSourceNode.h"
#include "OscillatorType.h"
#include "PeriodicWave.h"

namespace audioapi {

class OscillatorNode : public AudioScheduledSourceNode {
 public:
  explicit OscillatorNode(BaseAudioContext *context);

  [[nodiscard]] std::shared_ptr<AudioParam> getFrequencyParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
  [[nodiscard]] std::string getType();
  void setType(const std::string &type);

 protected:
  bool processAudio(float *audioData, int32_t numFrames) override;

 private:
  std::shared_ptr<AudioParam> frequencyParam_;
  std::shared_ptr<AudioParam> detuneParam_;
  OscillatorType type_;
  float phase_ = 0.0;
  std::shared_ptr<PeriodicWave> periodicWave_;

  static OscillatorType fromString(const std::string &type) {
    std::string lowerType = type;
    std::transform(
        lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

    if (lowerType == "sine")
      return OscillatorType::SINE;
    if (lowerType == "square")
      return OscillatorType::SQUARE;
    if (lowerType == "sawtooth")
      return OscillatorType::SAWTOOTH;
    if (lowerType == "triangle")
      return OscillatorType::TRIANGLE;
    if (lowerType == "custom")
      return OscillatorType::CUSTOM;

    throw std::invalid_argument("Unknown oscillator type: " + type);
  }

  static std::string toString(OscillatorType type) {
    switch (type) {
      case OscillatorType::SINE:
        return "sine";
      case OscillatorType::SQUARE:
        return "square";
      case OscillatorType::SAWTOOTH:
        return "sawtooth";
      case OscillatorType::TRIANGLE:
        return "triangle";
      case OscillatorType::CUSTOM:
        return "custom";
      default:
        throw std::invalid_argument("Unknown oscillator type");
    }
  }
};
} // namespace audioapi
