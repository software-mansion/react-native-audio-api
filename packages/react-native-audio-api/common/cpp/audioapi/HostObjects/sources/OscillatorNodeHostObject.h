#pragma once

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/types/OscillatorType.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct OscillatorOptions;
class BaseAudioContext;
class AudioParamHostObject;
class OscillatorNode;

class OscillatorNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit OscillatorNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const OscillatorOptions &options);

  JSI_PROPERTY_GETTER_DECL(frequency);
  JSI_PROPERTY_GETTER_DECL(detune);
  JSI_PROPERTY_GETTER_DECL(type);

  JSI_HOST_FUNCTION_DECL(setPeriodicWave);

  JSI_PROPERTY_SETTER_DECL(type);

  [[nodiscard]] size_t getMemoryPressure() const override {
    // frequency + detune params. Wavetables (PeriodicWave) are owned by a
    // separate PeriodicWaveHostObject when custom; default waveforms share
    // static tables, so we don't account for them here.
    return AudioNodeHostObject::getMemoryPressure() + 2 * kAudioParamBytes;
  }

 private:
  OscillatorNode *oscillatorNode_ = nullptr;

  std::shared_ptr<AudioParamHostObject> frequencyParam_;
  std::shared_ptr<AudioParamHostObject> detuneParam_;
  OscillatorType type_;
};
} // namespace audioapi
