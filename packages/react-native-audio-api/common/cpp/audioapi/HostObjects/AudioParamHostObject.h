#pragma once

#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/core/AudioParam.h>

#include <jsi/jsi.h>
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

  JSI_HOST_FUNCTION(setTargetAtTime) {
    auto target = static_cast<float>(args[0].getNumber());
    double startTime = args[1].getNumber();
    double timeConstant = args[2].getNumber();
    param_->setTargetAtTime(target, startTime, timeConstant);
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(setValueCurveAtTime) {
    auto arrayBuffer = args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
    auto values = reinterpret_cast<float *>(arrayBuffer.data(runtime));
    auto length = static_cast<int>(arrayBuffer.size(runtime));

    double startTime = args[1].getNumber();
    double duration = args[2].getNumber();
    param_->setValueCurveAtTime(values, length, startTime, duration);
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(cancelScheduledValues) {
    double cancelTime = args[0].getNumber();
    param_->cancelScheduledValues(cancelTime);
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(cancelAndHoldAtTime) {
    double cancelTime = args[0].getNumber();
    param_->cancelAndHoldAtTime(cancelTime);
    return jsi::Value::undefined();
  }

  JSI_PROPERTY_SETTER(value) {
    param_->setValue(static_cast<float>(value.getNumber()));
  }

 private:
  std::shared_ptr<AudioParam> param_;
};
} // namespace audioapi
