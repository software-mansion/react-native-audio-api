#include "AudioBufferSourceNodeHostObject.h"

namespace audioapi {
using namespace facebook;

AudioBufferSourceNodeHostObject::AudioBufferSourceNodeHostObject(
    const std::shared_ptr<AudioBufferSourceNodeWrapper> &wrapper)
    : AudioScheduledSourceNodeHostObject(wrapper) {}

std::shared_ptr<AudioBufferSourceNodeWrapper> AudioBufferSourceNodeHostObject::
    getAudioBufferSourceNodeWrapperFromAudioNodeWrapper() {
  return std::static_pointer_cast<AudioBufferSourceNodeWrapper>(wrapper_);
}

std::vector<jsi::PropNameID> AudioBufferSourceNodeHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames =
      AudioScheduledSourceNodeHostObject::getPropertyNames(runtime);
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "loop"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "buffer"));
  return propertyNames;
}

jsi::Value AudioBufferSourceNodeHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "loop") {
    auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
    auto loop = wrapper->getLoop();
    return {loop};
  }

  if (propName == "buffer") {
    auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
    auto buffer = wrapper->getBuffer();
    auto bufferHostObject = AudioBufferHostObject::createFromWrapper(buffer);
    return jsi::Object::createFromHostObject(runtime, bufferHostObject);
  }

  return AudioScheduledSourceNodeHostObject::get(runtime, propNameId);
}

void AudioBufferSourceNodeHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "loop") {
    auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
    wrapper->setLoop(value.getBool());
    return;
  }

  if (propName == "buffer") {
    auto bufferHostObject =
        value.getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
    auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
    wrapper->setBuffer(bufferHostObject->wrapper_);
    return;
  }

  AudioScheduledSourceNodeHostObject::set(runtime, propNameId, value);
}
} // namespace audioapi
