#include "AudioScheduledSourceNodeHostObject.h"

namespace audioapi {
using namespace facebook;

std::shared_ptr<AudioScheduledSourceNodeWrapper>
AudioScheduledSourceNodeHostObject::
    getAudioScheduledSourceNodeWrapperFromAudioNodeWrapper() {
  return std::static_pointer_cast<AudioScheduledSourceNodeWrapper>(wrapper_);
}

std::vector<jsi::PropNameID>
AudioScheduledSourceNodeHostObject::getPropertyNames(jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames =
      AudioNodeHostObject::getPropertyNames(runtime);
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "start"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "stop"));
  return propertyNames;
}

jsi::Value AudioScheduledSourceNodeHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "start") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        1,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto time = args[0].getNumber();
          auto wrapper =
              getAudioScheduledSourceNodeWrapperFromAudioNodeWrapper();
          wrapper->start(time);
          return jsi::Value::undefined();
        });
  }

  if (propName == "stop") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        1,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto time = args[0].getNumber();
          auto wrapper =
              getAudioScheduledSourceNodeWrapperFromAudioNodeWrapper();
          wrapper->stop(time);
          return jsi::Value::undefined();
        });
  }

  return AudioNodeHostObject::get(runtime, propNameId);
}

void AudioScheduledSourceNodeHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  return AudioNodeHostObject::set(runtime, propNameId, value);
}
} // namespace audioapi
