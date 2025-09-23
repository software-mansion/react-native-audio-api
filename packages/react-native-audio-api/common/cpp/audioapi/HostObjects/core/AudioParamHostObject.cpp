#include <audioapi/HostObjects/core/AudioParamHostObject.h>

namespace audioapi {

JSI_PROPERTY_GETTER_IMPL(AudioParamHostObject, value) {
  return {param_->getValue()};
}

JSI_PROPERTY_GETTER_IMPL(AudioParamHostObject, defaultValue) {
  return {param_->getDefaultValue()};
}

JSI_PROPERTY_GETTER_IMPL(AudioParamHostObject, minValue) {
  return {param_->getMinValue()};
}

JSI_PROPERTY_GETTER_IMPL(AudioParamHostObject, maxValue) {
  return {param_->getMaxValue()};
}

JSI_PROPERTY_SETTER_IMPL(AudioParamHostObject, value) {
  param_->setValue(static_cast<float>(value.getNumber()));
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, setValueAtTime) {
  auto value = static_cast<float>(args[0].getNumber());
  double startTime = args[1].getNumber();
  param_->setValueAtTime(value, startTime);
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, linearRampToValueAtTime) {
  auto value = static_cast<float>(args[0].getNumber());
  double endTime = args[1].getNumber();
  param_->linearRampToValueAtTime(value, endTime);
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, exponentialRampToValueAtTime) {
  auto value = static_cast<float>(args[0].getNumber());
  double endTime = args[1].getNumber();
  param_->exponentialRampToValueAtTime(value, endTime);
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, setTargetAtTime) {
  auto target = static_cast<float>(args[0].getNumber());
  double startTime = args[1].getNumber();
  double timeConstant = args[2].getNumber();
  param_->setTargetAtTime(target, startTime, timeConstant);
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, setValueCurveAtTime) {
  auto arrayBuffer = args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto rawValues = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime));
  auto values = std::make_unique<std::vector<float>>(rawValues, rawValues + length);

  double startTime = args[1].getNumber();
  double duration = args[2].getNumber();
  param_->setValueCurveAtTime(std::move(values), length, startTime, duration);
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, cancelScheduledValues) {
  double cancelTime = args[0].getNumber();
  param_->cancelScheduledValues(cancelTime);
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioParamHostObject, cancelAndHoldAtTime) {
  double cancelTime = args[0].getNumber();
  param_->cancelAndHoldAtTime(cancelTime);
  return jsi::Value::undefined();
}

} // namespace audioapi
