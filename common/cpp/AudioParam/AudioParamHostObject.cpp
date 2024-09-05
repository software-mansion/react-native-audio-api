#include "AudioParamHostObject.h"

namespace audioapi {
using namespace facebook;

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
    return jsi::Value(wrapper_->getValue());
  }

  if (propName == "defaultValue") {
    return jsi::Value(wrapper_->getDefaultValue());
  }

  if (propName == "minValue") {
    return jsi::Value(wrapper_->getMinValue());
  }

  if (propName == "maxValue") {
    return jsi::Value(wrapper_->getMaxValue());
  }

  if (propName == "defaultValue") {
    return jsi::Value(wrapper_->getDefaultValue());
  }

  if (propName == "minValue") {
    return jsi::Value(wrapper_->getMinValue());
  }

  if (propName == "maxValue") {
    return jsi::Value(wrapper_->getMaxValue());
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
          double value = args[0].getNumber();
          double startTime = args[1].getNumber();
          wrapper_->setValueAtTime(value, startTime);
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
          double value = args[0].getNumber();
          double endTime = args[1].getNumber();
          wrapper_->linearRampToValueAtTime(value, endTime);
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
          double value = args[0].getNumber();
          double endTime = args[1].getNumber();
          wrapper_->linearRampToValueAtTime(value, endTime);
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
    double paramValue = value.getNumber();
    wrapper_->setValue(paramValue);
    return;
  }

  throw std::runtime_error("Not yet implemented!");
}

} // namespace audioapi
