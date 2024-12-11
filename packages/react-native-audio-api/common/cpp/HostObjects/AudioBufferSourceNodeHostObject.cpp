#include "AudioBufferSourceNodeHostObject.h"

namespace audioapi {
using namespace facebook;

AudioBufferSourceNodeHostObject::AudioBufferSourceNodeHostObject(
    const std::shared_ptr<AudioBufferSourceNode> &node)
    : AudioScheduledSourceNodeHostObject(node) {}

std::shared_ptr<AudioBufferSourceNode>
AudioBufferSourceNodeHostObject::getAudioBufferSourceNodeFromAudioNode() {
  return std::static_pointer_cast<AudioBufferSourceNode>(node_);
}

std::vector<jsi::PropNameID> AudioBufferSourceNodeHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames =
      AudioScheduledSourceNodeHostObject::getPropertyNames(runtime);
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "loop"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "buffer"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "resetBuffer"));
  return propertyNames;
}

jsi::Value AudioBufferSourceNodeHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "loop") {
    auto wrapper = getAudioBufferSourceNodeFromAudioNode();
    auto loop = wrapper->getLoop();
    return {loop};
  }

  if (propName == "buffer") {
    auto wrapper = getAudioBufferSourceNodeFromAudioNode();
    auto buffer = wrapper->getBuffer();

    if (!buffer) {
      return jsi::Value::null();
    }

    auto bufferHostObject = std::make_shared<AudioBufferHostObject>(buffer);
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
    auto wrapper = getAudioBufferSourceNodeFromAudioNode();
    wrapper->setLoop(value.getBool());
    return;
  }

  if (propName == "buffer") {
    auto bufferSource = getAudioBufferSourceNodeFromAudioNode();

    if (value.isNull()) {
      bufferSource->setBuffer(std::shared_ptr<AudioBuffer>(nullptr));
      return;
    }

    auto bufferHostObject =
        value.getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
    bufferSource->setBuffer(bufferHostObject->audioBuffer_);
    return;
  }

  AudioScheduledSourceNodeHostObject::set(runtime, propNameId, value);
}
} // namespace audioapi
