#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/types/OscillatorType.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

struct OscillatorOptions;
class BaseAudioContext;

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
};
} // namespace audioapi
