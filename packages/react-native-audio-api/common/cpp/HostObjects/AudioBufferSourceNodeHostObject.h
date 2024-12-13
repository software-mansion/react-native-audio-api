#pragma once

#include <memory>
#include <vector>

#include "AudioBufferHostObject.h"
#include "AudioBufferSourceNode.h"
#include "AudioParamHostObject.h"
#include "AudioScheduledSourceNodeHostObject.h"

namespace audioapi {
using namespace facebook;

class AudioBufferSourceNodeHostObject
    : public AudioScheduledSourceNodeHostObject {
 public:
  explicit AudioBufferSourceNodeHostObject(
      const std::shared_ptr<AudioBufferSourceNode> &node)
      : AudioScheduledSourceNodeHostObject(node) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loop),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, buffer),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopStart),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopEnd),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, detune),
        JSI_EXPORT_PROPERTY_GETTER(
            AudioBufferSourceNodeHostObject, playbackRate));
    addSetters(
        JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loop),
        JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, buffer));
  }

  JSI_PROPERTY_GETTER(loop) {
    auto audioBufferSourceNode =
        std::static_pointer_cast<AudioBufferSourceNode>(node_);
    auto loop = audioBufferSourceNode->getLoop();
    return {loop};
  }

  JSI_PROPERTY_GETTER(buffer) {
    auto audioBufferSourceNode =
        std::static_pointer_cast<AudioBufferSourceNode>(node_);
    auto buffer = audioBufferSourceNode->getBuffer();

    if (!buffer) {
      return jsi::Value::null();
    }

    auto bufferHostObject = std::make_shared<AudioBufferHostObject>(buffer);
    return jsi::Object::createFromHostObject(runtime, bufferHostObject);
  }

  JSI_PROPERTY_GETTER(loopStart) {
    auto audioBufferSourceNode =
        std::static_pointer_cast<AudioBufferSourceNode>(node_);
    auto loopStart = audioBufferSourceNode->getLoopStart();
    return {loopStart};
  }

  JSI_PROPERTY_GETTER(loopEnd) {
    auto audioBufferSourceNode =
        std::static_pointer_cast<AudioBufferSourceNode>(node_);
    auto loopEnd = audioBufferSourceNode->getLoopEnd();
    return {loopEnd};
  }

  JSI_PROPERTY_GETTER(detune) {
    auto audioBufferSourceNode =
        std::static_pointer_cast<AudioBufferSourceNode>(node_);
    auto detune = audioBufferSourceNode->getDetuneParam();
    auto detuneHostObject = std::make_shared<AudioParamHostObject>(detune);
    return jsi::Object::createFromHostObject(runtime, detuneHostObject);
  }

  JSI_PROPERTY_GETTER(playbackRate) {
    auto audioBufferSourceNode =
        std::static_pointer_cast<AudioBufferSourceNode>(node_);
    auto playbackRate = audioBufferSourceNode->getPlaybackRateParam();
    auto playbackRateHostObject =
        std::make_shared<AudioParamHostObject>(playbackRate);
    return jsi::Object::createFromHostObject(runtime, playbackRateHostObject);
  }

  JSI_PROPERTY_SETTER(loop) {
    auto audioBufferSourceNode =
        std::static_pointer_cast<AudioBufferSourceNode>(node_);
    audioBufferSourceNode->setLoop(value.getBool());
  }

  JSI_PROPERTY_SETTER(buffer) {
    auto audioBufferSourceNode =
        std::static_pointer_cast<AudioBufferSourceNode>(node_);
    if (value.isNull()) {
      audioBufferSourceNode->setBuffer(std::shared_ptr<AudioBuffer>(nullptr));
      return;
    }

    auto bufferHostObject =
        value.getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
    audioBufferSourceNode->setBuffer(bufferHostObject->audioBuffer_);
  }

  JSI_PROPERTY_SETTER(loopStart) {
    auto audioBufferSourceNode =
        std::static_pointer_cast<AudioBufferSourceNode>(node_);
    audioBufferSourceNode->setLoopStart(value.getNumber());
  }

  JSI_PROPERTY_SETTER(loopEnd) {
    auto audioBufferSourceNode =
        std::static_pointer_cast<AudioBufferSourceNode>(node_);
    audioBufferSourceNode->setLoopEnd(value.getNumber());
  }
};

} // namespace audioapi
