#include "JsiHostObject.h"

namespace audioapi {
JsiHostObject::JsiHostObject() {
  propertyGetters_ = std::make_unique<std::unordered_map<
      std::string,
      jsi::Value (JsiHostObject::*)(jsi::Runtime &)>>();
  propertyFunctions_ = std::make_unique<std::unordered_map<
      std::string,
      jsi::Value (JsiHostObject::*)(
          jsi::Runtime &, const jsi::Value &, const jsi::Value *, size_t)>>();
  propertySetters_ = std::make_unique<std::unordered_map<
      std::string,
      void (JsiHostObject::*)(jsi::Runtime &, const jsi::Value &)>>();

  propertyGetters_->insert(JSI_EXPORT_PROPERTY_GETTER(JsiHostObject, then));
}

std::vector<jsi::PropNameID> JsiHostObject::getPropertyNames(jsi::Runtime &rt) {
  std::vector<jsi::PropNameID> propertyNames;
  propertyNames.reserve(
      propertyGetters_->size() + propertyFunctions_->size() +
      propertySetters_->size());

  for (const auto &it : *propertyGetters_) {
    propertyNames.push_back(jsi::PropNameID::forUtf8(rt, it.first));
  }

  for (const auto &it : *propertyFunctions_) {
    propertyNames.push_back(jsi::PropNameID::forAscii(rt, it.first));
  }

  for (const auto &it : *propertySetters_) {
    propertyNames.push_back(jsi::PropNameID::forAscii(rt, it.first));
  }

  return propertyNames;
}

jsi::Value JsiHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &name) {
  auto nameAsString = name.utf8(runtime);

  auto propertyGetter = propertyGetters_->find(nameAsString);
  if (propertyGetter != propertyGetters_->end()) {
    auto dispatcher = [this, &propertyGetter](jsi::Runtime &runtime) {
      return (this->*(propertyGetter->second))(runtime);
    };

    return dispatcher(runtime);
  }

  auto propertyFunction = propertyFunctions_->find(nameAsString);
  if (propertyFunction != propertyFunctions_->end()) {
    auto dispatcher = [this, &propertyFunction](
                          jsi::Runtime &runtime,
                          const jsi::Value &thisVal,
                          const jsi::Value *args,
                          size_t count) -> jsi::Value {
      return (this->*(propertyFunction->second))(runtime, thisVal, args, count);
    };

    return jsi::Function::createFromHostFunction(runtime, name, 0, dispatcher);
  }

  return jsi::Value::undefined();
}

void JsiHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &name,
    const jsi::Value &value) {
  auto nameAsString = name.utf8(runtime);

  auto propertySetter = propertySetters_->find(nameAsString);

  if (propertySetter != propertySetters_->end()) {
    auto dispatcher = [this, &propertySetter](
                          jsi::Runtime &runtime, const jsi::Value &value) {
      return (this->*(propertySetter->second))(runtime, value);
    };

    return dispatcher(runtime, value);
  }
}
} // namespace audioapi
