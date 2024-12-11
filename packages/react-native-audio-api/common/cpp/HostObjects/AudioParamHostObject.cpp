#include "AudioParamHostObject.h"

namespace audioapi {
using namespace facebook;

AudioParamHostObject::AudioParamHostObject(
    const std::shared_ptr<AudioParam> &param)
    : param_(param) {}

std::vector<jsi::PropNameID> AudioParamHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames;
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "value"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "defaultValue"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "minValue"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "maxValue"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "setValueAtTime"));
  propertyNames.push_back(
      jsi::PropNameID::forAscii(runtime, "linearRampToValueAtTime"));
  propertyNames.push_back(
      jsi::PropNameID::forAscii(runtime, "exponentialRampToValueAtTime"));
  return propertyNames;
}

jsi::Value AudioParamHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "value") {
    return {param_->getValue()};
  }

  if (propName == "defaultValue") {
    return {param_->getDefaultValue()};
  }

  if (propName == "minValue") {
    return {param_->getMinValue()};
  }

  if (propName == "maxValue") {
    return {param_->getMaxValue()};
  }

  if (propName == "setValueAtTime") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        2,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisVal,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
            auto value = static_cast<float>(args[0].getNumber());
          double startTime = args[1].getNumber();
          param_->setValueAtTime(value, startTime);
          return jsi::Value::undefined();
        });
  }

  if (propName == "linearRampToValueAtTime") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        2,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisVal,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
            auto value = static_cast<float>(args[0].getNumber());
          double endTime = args[1].getNumber();
          param_->linearRampToValueAtTime(value, endTime);
          return jsi::Value::undefined();
        });
  }

  if (propName == "exponentialRampToValueAtTime") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        2,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisVal,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto value = static_cast<float>(args[0].getNumber());
          double endTime = args[1].getNumber();
          param_->exponentialRampToValueAtTime(value, endTime);
          return jsi::Value::undefined();
        });
  }

  throw std::runtime_error("Not yet implemented!");
}

void AudioParamHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "value") {
    auto paramValue = static_cast<float>(value.getNumber());
    param_->setValue(paramValue);
    return;
  }

  throw std::runtime_error("Not yet implemented!");
}

} // namespace audioapi
