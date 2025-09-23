#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/effects/BiquadFilterNode.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class BiquadFilterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit BiquadFilterNodeHostObject(
      const std::shared_ptr<BiquadFilterNode> &node)
      : AudioNodeHostObject(node) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, frequency),
        JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, detune),
        JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, Q),
        JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, gain),
        JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, type));

    addSetters(JSI_EXPORT_PROPERTY_SETTER(BiquadFilterNodeHostObject, type));

    addFunctions(
        JSI_EXPORT_FUNCTION(BiquadFilterNodeHostObject, getFrequencyResponse));
  }

  JSI_PROPERTY_GETTER_DECL(frequency);
  JSI_PROPERTY_GETTER_DECL(detune);
  JSI_PROPERTY_GETTER_DECL(Q);
  JSI_PROPERTY_GETTER_DECL(gain);
  JSI_PROPERTY_GETTER_DECL(type);

  JSI_PROPERTY_SETTER_DECL(type);

  JSI_HOST_FUNCTION_DECL(getFrequencyResponse);
};
} // namespace audioapi
