#include "AudioContextHostObject.h"

namespace audioapi {
using namespace facebook;

AudioContextHostObject::AudioContextHostObject(
    const std::shared_ptr<AudioContextWrapper> &wrapper, std::shared_ptr<JsiPromise::PromiseVendor> promiseVendor)
    : BaseAudioContextHostObject(wrapper, promiseVendor) {}

std::vector<jsi::PropNameID> AudioContextHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames =
      BaseAudioContextHostObject::getPropertyNames(runtime);
  propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "close"));
  return propertyNames;
}

jsi::Value AudioContextHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "close") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        0,
        [this](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          getAudioContextWrapperFromBaseAudioContextWrapper()->close();
          return jsi::Value::undefined();
        });
  }

  return BaseAudioContextHostObject::get(runtime, propNameId);
}

void AudioContextHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  return BaseAudioContextHostObject::set(runtime, propNameId, value);
}

std::shared_ptr<AudioContextWrapper>
AudioContextHostObject::getAudioContextWrapperFromBaseAudioContextWrapper() {
  return std::static_pointer_cast<AudioContextWrapper>(wrapper_);
}
} // namespace audioapi
