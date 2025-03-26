#pragma once

#include <audioapi/HostObjects/AudioBufferHostObject.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/AudioScheduledSourceNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioBufferSourceNodeHostObject
    : public AudioScheduledSourceNodeHostObject {
 public:
  explicit AudioBufferSourceNodeHostObject(
      const std::shared_ptr<AudioBufferSourceNode> &node,
      const std::shared_ptr<react::CallInvoker> &callInvoker)
      : AudioScheduledSourceNodeHostObject(node, callInvoker) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loop),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, buffer),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopStart),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopEnd),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, detune),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, playbackRate));

    addSetters(
        JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loop),
        JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, buffer),
        JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loopStart),
        JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loopEnd));

    // start method is overridden in this class
    functions_->erase("start");

    addFunctions(
        JSI_EXPORT_FUNCTION(AudioBufferSourceNodeHostObject, start));
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

  JSI_HOST_FUNCTION(start) {
    auto when = args[0].getNumber();
    auto offset = args[1].getNumber();

    auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);

    if (args[2].isUndefined()) {
        audioBufferSourceNode->start(when, offset);

        return jsi::Value::undefined();
    }

    auto duration = args[2].getNumber();
    audioBufferSourceNode->start(when, offset, duration);

    return jsi::Value::undefined();
  }
};

} // namespace audioapi
