#include <audioapi/HostObjects/AudioParamHostObject.h>

#include <audioapi/core/AudioParam.h>
#include <audioapi/utils/AudioArray.h>

#include <memory>
#include <utility>

namespace audioapi {

AudioParamHostObject::AudioParamHostObject(const std::shared_ptr<AudioParam> &param)
    : param_(param),
      defaultValue_(param->getDefaultValue()),
      minValue_(param->getMinValue()),
      maxValue_(param->getMaxValue()) {
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

JSI_PROPERTY_GETTER_IMPL(AudioParamHostObject, value) {
  return {param_->getValue()};
}

JSI_PROPERTY_GETTER_IMPL(AudioParamHostObject, defaultValue) {
  return {defaultValue_};
}

JSI_PROPERTY_GETTER_IMPL(AudioParamHostObject, minValue) {
  return {minValue_};
}

JSI_PROPERTY_GETTER_IMPL(AudioParamHostObject, maxValue) {
  return {maxValue_};
}

JSI_PROPERTY_SETTER_IMPL(AudioParamHostObject, value) {
  auto event = [param = param_, value = static_cast<float>(value.getNumber())](BaseAudioContext &) {
    param->setValue(value);
  };

  param_->scheduleAudioEvent(std::move(event));
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, setValueAtTime) {
  auto event = [param = param_,
                value = static_cast<float>(args[0].getNumber()),
                startTime = args[1].getNumber()](BaseAudioContext &) {
    param->setValueAtTime(value, startTime);
  };

  param_->scheduleAudioEvent(std::move(event));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, linearRampToValueAtTime) {
  auto event = [param = param_,
                value = static_cast<float>(args[0].getNumber()),
                endTime = args[1].getNumber()](BaseAudioContext &) {
    param->linearRampToValueAtTime(value, endTime);
  };

  param_->scheduleAudioEvent(std::move(event));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, exponentialRampToValueAtTime) {
  auto event = [param = param_,
                value = static_cast<float>(args[0].getNumber()),
                endTime = args[1].getNumber()](BaseAudioContext &) {
    param->exponentialRampToValueAtTime(value, endTime);
  };

  param_->scheduleAudioEvent(std::move(event));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, setTargetAtTime) {
  auto event = [param = param_,
                target = static_cast<float>(args[0].getNumber()),
                startTime = args[1].getNumber(),
                timeConstant = args[2].getNumber()](BaseAudioContext &) {
    param->setTargetAtTime(target, startTime, timeConstant);
  };

  param_->scheduleAudioEvent(std::move(event));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, setValueCurveAtTime) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto rawValues = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime) / sizeof(float));
  auto values = std::make_shared<AudioArray>(rawValues, length);

  auto event = [param = param_,
                values,
                length,
                startTime = args[1].getNumber(),
                duration = args[2].getNumber()](BaseAudioContext &) {
    param->setValueCurveAtTime(values, length, startTime, duration);
  };

  param_->scheduleAudioEvent(std::move(event));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, cancelScheduledValues) {
  auto event = [param = param_, cancelTime = args[0].getNumber()](BaseAudioContext &) {
    param->cancelScheduledValues(cancelTime);
  };

  param_->scheduleAudioEvent(std::move(event));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, cancelAndHoldAtTime) {
  auto event = [param = param_, cancelTime = args[0].getNumber()](BaseAudioContext &) {
    param->cancelAndHoldAtTime(cancelTime);
  };

  param_->scheduleAudioEvent(std::move(event));
  return jsi::Value::undefined();
}

} // namespace audioapi
