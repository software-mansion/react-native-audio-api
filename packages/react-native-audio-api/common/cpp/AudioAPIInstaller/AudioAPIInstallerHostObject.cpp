#include "AudioAPIInstallerHostObject.h"

namespace audioapi {
using namespace facebook;

AudioAPIInstallerHostObject::AudioAPIInstallerHostObject(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &jsInvoker)
    : rnRuntime_(runtime) {
  promiseVendor_ =
      std::make_shared<JsiPromise::PromiseVendor>(runtime, jsInvoker);
}

std::vector<jsi::PropNameID> AudioAPIInstallerHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames;
  propertyNames.push_back(
      jsi::PropNameID::forUtf8(runtime, "createAudioContext"));
  return propertyNames;
}

jsi::Value AudioAPIInstallerHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "createAudioContext") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        0,
        [this](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          auto audioContext = std::make_shared<AudioContext>();
          auto audioContextHostObject =
              std::make_shared<AudioContextHostObject>(
                  audioContext, promiseVendor_);
          return jsi::Object::createFromHostObject(
              runtime, audioContextHostObject);
        });
  }

  throw std::runtime_error("Not yet implemented!");
}

void AudioAPIInstallerHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  throw std::runtime_error("Not yet implemented!");
}
} // namespace audioapi
