#pragma once

#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/core/core/AudioParam.h>

#include <jsi/jsi.h>
#include <utility>
#include <memory>
#include <vector>
#include <cstddef>

namespace audioapi {
using namespace facebook;

class AudioParamHostObject : public JsiHostObject {
 public:
  explicit AudioParamHostObject(const std::shared_ptr<AudioParam> &param)
      : param_(param) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(AudioParamHostObject, value),
        JSI_EXPORT_PROPERTY_GETTER(AudioParamHostObject, defaultValue),
        JSI_EXPORT_PROPERTY_GETTER(AudioParamHostObject, minValue),
        JSI_EXPORT_PROPERTY_GETTER(AudioParamHostObject, maxValue));

    addFunctions(
        JSI_EXPORT_FUNCTION(AudioParamHostObject, setValueAtTime),
        JSI_EXPORT_FUNCTION(AudioParamHostObject, linearRampToValueAtTime),
        JSI_EXPORT_FUNCTION(AudioParamHostObject, exponentialRampToValueAtTime),
        JSI_EXPORT_FUNCTION(AudioParamHostObject, setTargetAtTime),
        JSI_EXPORT_FUNCTION(AudioParamHostObject, setValueCurveAtTime),
        JSI_EXPORT_FUNCTION(AudioParamHostObject, cancelScheduledValues),
        JSI_EXPORT_FUNCTION(AudioParamHostObject, cancelAndHoldAtTime));

    addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioParamHostObject, value));
  }
  friend class AudioNodeHostObject;

  JSI_PROPERTY_GETTER_DECL(value);
  JSI_PROPERTY_GETTER_DECL(defaultValue);
  JSI_PROPERTY_GETTER_DECL(minValue);
  JSI_PROPERTY_GETTER_DECL(maxValue);

  JSI_PROPERTY_SETTER_DECL(value);

  JSI_HOST_FUNCTION_DECL(setValueAtTime);
  JSI_HOST_FUNCTION_DECL(linearRampToValueAtTime);
  JSI_HOST_FUNCTION_DECL(exponentialRampToValueAtTime);
  JSI_HOST_FUNCTION_DECL(setTargetAtTime);
  JSI_HOST_FUNCTION_DECL(setValueCurveAtTime);
  JSI_HOST_FUNCTION_DECL(cancelScheduledValues);
  JSI_HOST_FUNCTION_DECL(cancelAndHoldAtTime);

 private:
  std::shared_ptr<AudioParam> param_;
};
} // namespace audioapi
