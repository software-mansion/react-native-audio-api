#pragma once

#include <cmath>
#include <memory>
#include <string>

#include "AudioParam.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {

    //TODO implement OscillatorNode

using namespace oboe;

class OscillatorNode : public AudioStreamDataCallback,
                       public AudioScheduledSourceNode {
 public:
  explicit OscillatorNode(AudioContext *context);

  std::shared_ptr<AudioParam> getFrequencyParam() const;
  std::shared_ptr<AudioParam> getDetuneParam() const;
  std::string getType();
  void setType(const std::string &type);

  DataCallbackResult onAudioReady(
      AudioStream *oboeStream,
      void *audioData,
      int32_t numFrames) override;

private:
    enum class WaveType {
        SINE,
        SQUARE,
        SAWTOOTH,
        TRIANGLE
    };

    static float sineWave(double wavePhase) {
        return std::sin(wavePhase);
    }

    static float squareWave(double wavePhase) {
        return std::sin(wavePhase) >= 0 ? 1.0f : -1.0f;
    }

    static float sawtoothWave(double wavePhase) {
        return 2.0f * (wavePhase / (2 * M_PI) - std::floor(wavePhase / (2 * M_PI) + 0.5));
    }

    static float triangleWave(double wavePhase) {
        return 2.0f * std::abs(2.0 * (wavePhase / (2 * M_PI) - std::floor(wavePhase / (2 * M_PI) + 0.5))) - 1.0f;
    }

    static float getWaveValue(double wavePhase, WaveType type) {
        switch (type) {
            case WaveType::SINE:
                return sineWave(wavePhase);
            case WaveType::SQUARE:
                return squareWave(wavePhase);
            case WaveType::SAWTOOTH:
                return sawtoothWave(wavePhase);
            case WaveType::TRIANGLE:
                return triangleWave(wavePhase);
            default:
                throw std::invalid_argument("Unknown wave type");
        }
    }

    static WaveType fromString(const std::string& type) {
        std::string lowerType = type;
        std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

        if (lowerType == "sine") return WaveType::SINE;
        if (lowerType == "square") return WaveType::SQUARE;
        if (lowerType == "sawtooth") return WaveType::SAWTOOTH;
        if (lowerType == "triangle") return WaveType::TRIANGLE;

        throw std::invalid_argument("Unknown wave type: " + type);
    }

    static std::string toString(WaveType type) {
        switch (type) {
            case WaveType::SINE: return "sine";
            case WaveType::SQUARE: return "square";
            case WaveType::SAWTOOTH: return "sawtooth";
            case WaveType::TRIANGLE: return "triangle";
            default:
                throw std::invalid_argument("Unknown wave type");
        }
    }

    static float getWaveBufferElement(double wavePhase, WaveType waveType) {
        return getWaveValue(wavePhase, waveType);
    }

 private:
  std::shared_ptr<AudioParam> frequencyParam_;
  std::shared_ptr<AudioParam> detuneParam_;
  WaveType type_ = WaveType::SINE;
  float phase_ = 0.0;
};
} // namespace audioapi
