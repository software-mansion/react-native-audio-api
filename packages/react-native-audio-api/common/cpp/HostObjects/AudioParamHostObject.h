#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include <JsiHostObject.h>
#include "AudioParam.h"

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
        JSI_EXPORT_FUNCTION(
            AudioParamHostObject, exponentialRampToValueAtTime));

    addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioParamHostObject, value));
  }

  JSI_PROPERTY_GETTER(value) {
    return {param_->getValue()};
  }

  JSI_PROPERTY_GETTER(defaultValue) {
    return {param_->getDefaultValue()};
  }

  JSI_PROPERTY_GETTER(minValue) {
    return {param_->getMinValue()};
  }

  JSI_PROPERTY_GETTER(maxValue) {
    return {param_->getMaxValue()};
  }

  JSI_HOST_FUNCTION(setValueAtTime) {
    auto value = static_cast<float>(args[0].getNumber());
    double startTime = args[1].getNumber();
    param_->setValueAtTime(value, startTime);
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(linearRampToValueAtTime) {
    auto value = static_cast<float>(args[0].getNumber());
    double endTime = args[1].getNumber();
    param_->linearRampToValueAtTime(value, endTime);
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(exponentialRampToValueAtTime) {
    auto value = static_cast<float>(args[0].getNumber());
    double endTime = args[1].getNumber();
    param_->exponentialRampToValueAtTime(value, endTime);
    return jsi::Value::undefined();
  }

  JSI_PROPERTY_SETTER(value) {
    param_->setValue(static_cast<float>(value.getNumber()));
  }

 private:
  std::shared_ptr<AudioParam> param_;
};
} // namespace audioapi
