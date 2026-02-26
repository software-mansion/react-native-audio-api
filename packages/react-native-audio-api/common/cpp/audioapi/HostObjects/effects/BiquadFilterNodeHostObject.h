#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct BiquadFilterOptions;
class BaseAudioContext;
class AudioParamHostObject;

class BiquadFilterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit BiquadFilterNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const BiquadFilterOptions &options);

  JSI_PROPERTY_GETTER_DECL(frequency);
  JSI_PROPERTY_GETTER_DECL(detune);
  JSI_PROPERTY_GETTER_DECL(Q);
  JSI_PROPERTY_GETTER_DECL(gain);
  JSI_PROPERTY_GETTER_DECL(type);

  JSI_PROPERTY_SETTER_DECL(type);

  JSI_HOST_FUNCTION_DECL(getFrequencyResponse);

 private:
  std::shared_ptr<AudioParamHostObject> frequencyParam_;
  std::shared_ptr<AudioParamHostObject> detuneParam_;
  std::shared_ptr<AudioParamHostObject> QParam_;
  std::shared_ptr<AudioParamHostObject> gainParam_;

  BiquadFilterType type_;
};
} // namespace audioapi
