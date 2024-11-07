#pragma once

#include <cmath>
#include <memory>
#include <string>

#include "AudioParam.h"
#include "AudioScheduledSourceNode.h"
#include "OscillatorType.h"

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

  static float sineWave(double wavePhase) {
    return static_cast<float>(std::sin(wavePhase));
  }

  static float squareWave(double wavePhase) {
    return static_cast<float>(std::sin(wavePhase) >= 0 ? 1.0 : -1.0);
  }

  static float sawtoothWave(double wavePhase) {
    return static_cast<float>(
        2.0 *
        (wavePhase / (2 * M_PI) - std::floor(wavePhase / (2 * M_PI) + 0.5)));
  }

  static float triangleWave(double wavePhase) {
    return static_cast<float>(
        2.0 *
            std::abs(
                2.0 *
                (wavePhase / (2 * M_PI) -
                 std::floor(wavePhase / (2 * M_PI) + 0.5))) -
        1.0);
  }

  static float getWaveBufferElement(double wavePhase,OscillatorType type) {
      switch (type) {
          case OscillatorType::SINE:
              return sineWave(wavePhase);
          case OscillatorType::SQUARE:
              return squareWave(wavePhase);
          case OscillatorType::SAWTOOTH:
              return sawtoothWave(wavePhase);
          case OscillatorType::TRIANGLE:
              return triangleWave(wavePhase);
          default:
              throw std::invalid_argument("Unknown wave type");
      }
  }

 private:
  std::shared_ptr<AudioParam> frequencyParam_;
  std::shared_ptr<AudioParam> detuneParam_;
  OscillatorType type_ = OscillatorType::SINE;
  float phase_ = 0.0;

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
