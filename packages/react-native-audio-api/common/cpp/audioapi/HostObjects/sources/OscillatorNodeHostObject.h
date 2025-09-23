#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/HostObjects/core/AudioParamHostObject.h>
#include <audioapi/HostObjects/effects/PeriodicWaveHostObject.h>
#include <audioapi/core/sources/OscillatorNode.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class OscillatorNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit OscillatorNodeHostObject(
          const std::shared_ptr<OscillatorNode> &node)
      : AudioScheduledSourceNodeHostObject(node) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(OscillatorNodeHostObject, frequency),
        JSI_EXPORT_PROPERTY_GETTER(OscillatorNodeHostObject, detune),
        JSI_EXPORT_PROPERTY_GETTER(OscillatorNodeHostObject, type));

    addFunctions(
        JSI_EXPORT_FUNCTION(OscillatorNodeHostObject, setPeriodicWave));

    addSetters(JSI_EXPORT_PROPERTY_SETTER(OscillatorNodeHostObject, type));
  }

  JSI_PROPERTY_GETTER_DECL(frequency);
  JSI_PROPERTY_GETTER_DECL(detune);
  JSI_PROPERTY_GETTER_DECL(type);

  JSI_HOST_FUNCTION_DECL(setPeriodicWave);

  JSI_PROPERTY_SETTER_DECL(type);
};
} // namespace audioapi
