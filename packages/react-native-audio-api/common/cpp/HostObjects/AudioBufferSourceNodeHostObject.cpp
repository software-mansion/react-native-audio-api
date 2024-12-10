#include "AudioBufferSourceNodeHostObject.h"
#include "AudioParamHostObject.h"

namespace audioapi {
using namespace facebook;

AudioBufferSourceNodeHostObject::AudioBufferSourceNodeHostObject(
    const std::shared_ptr<AudioBufferSourceNodeWrapper> &wrapper)
    : AudioScheduledSourceNodeHostObject(wrapper) {
  auto detuneParam = wrapper->getDetuneParam();
  detuneParam_ = AudioParamHostObject::createFromWrapper(detuneParam);
  auto playbackRateParam = wrapper->getPlaybackRateParam();
  playbackRateParam_ = AudioParamHostObject::createFromWrapper(playbackRateParam);
}

std::shared_ptr<AudioBufferSourceNodeWrapper> AudioBufferSourceNodeHostObject::
    getAudioBufferSourceNodeWrapperFromAudioNodeWrapper() {
  return std::static_pointer_cast<AudioBufferSourceNodeWrapper>(wrapper_);
}

std::vector<jsi::PropNameID> AudioBufferSourceNodeHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames =
      AudioScheduledSourceNodeHostObject::getPropertyNames(runtime);
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "loop"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "loopStart"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "loopEnd"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "detune"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "playbackRate"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "buffer"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "resetBuffer"));
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

  if (propName == "loopStart") {
    auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
    auto loopStart = wrapper->getLoopStart();
    return {loopStart};
  }

  if (propName == "loopEnd") {
    auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
    auto loopEnd = wrapper->getLoopEnd();
    return {loopEnd};
  }

  if (propName == "detune") {
    return jsi::Object::createFromHostObject(runtime, detuneParam_);
  }

  if (propName == "playbackRate") {
    return jsi::Object::createFromHostObject(runtime, playbackRateParam_);
  }

  if (propName == "buffer") {
    auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
    auto buffer = wrapper->getBuffer();

    if (!buffer) {
      return jsi::Value::null();
    }

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

  if (propName == "loopStart") {
    auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
    wrapper->setLoopStart(value.getNumber());
    return;
  }

  if (propName == "loopEnd") {
    auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
    wrapper->setLoopEnd(value.getNumber());
    return;
  }

  if (propName == "buffer") {
    auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();

    if (value.isNull()) {
      wrapper->setBuffer(std::shared_ptr<AudioBufferWrapper>(nullptr));
      return;
    }

    auto bufferHostObject =
        value.getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
    wrapper->setBuffer(bufferHostObject->wrapper_);
    return;
  }

  AudioScheduledSourceNodeHostObject::set(runtime, propNameId, value);
}
} // namespace audioapi
