#include <audioapi/HostObjects/sources/AudioBufferQueueSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/utils/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>

#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AudioBufferQueueSourceNodeHostObject::AudioBufferQueueSourceNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const BaseAudioBufferSourceOptions &options)
    : AudioBufferBaseSourceNodeHostObject(context->createBufferQueueSource(options)) {
  functions_->erase("start");

  addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioBufferQueueSourceNodeHostObject, onBufferEnded));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, start),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, enqueueBuffer),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, dequeueBuffer),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, clearBuffers),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, pause));
}

AudioBufferQueueSourceNodeHostObject::~AudioBufferQueueSourceNodeHostObject() {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  // When JSI object is garbage collected (together with the eventual callback),
  // underlying source node might still be active and try to call the
  // non-existing callback.
  audioBufferQueueSourceNode->setOnBufferEndedCallbackId(0);
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferQueueSourceNodeHostObject, onBufferEnded) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  audioBufferQueueSourceNode->setOnBufferEndedCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, start) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto event = [
        audioBufferQueueSourceNode,
        when = args[0].getNumber(),
        offset = args[1].isNumber() ? args[1].getNumber() : -1
  ](BaseAudioContext &) {
      if (offset >= 0) {
          audioBufferQueueSourceNode->start(when, offset);
      } else {
          audioBufferQueueSourceNode->start(when);
      }
  };

    audioBufferQueueSourceNode->scheduleAudioEvent(std::move(event));

    return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, pause) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto event = [audioBufferQueueSourceNode](BaseAudioContext &) {
      audioBufferQueueSourceNode->pause();
  };

  audioBufferQueueSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, enqueueBuffer) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

    auto audioBufferHostObject =
            args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);

  auto event = [audioBufferQueueSourceNode, buffer = audioBufferHostObject->audioBuffer_] (BaseAudioContext &) {
    audioBufferQueueSourceNode->enqueueBuffer(buffer);
  };

  audioBufferQueueSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::String::createFromUtf8(runtime, std::to_string(bufferId_++));
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, dequeueBuffer) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto event = [audioBufferQueueSourceNode, bufferId = static_cast<size_t>(args[0].getNumber())] (BaseAudioContext &) {
        audioBufferQueueSourceNode->dequeueBuffer(bufferId);
    };

  audioBufferQueueSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, clearBuffers) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto event = [audioBufferQueueSourceNode] (BaseAudioContext &) {
        audioBufferQueueSourceNode->clearBuffers();
    };

  audioBufferQueueSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

} // namespace audioapi
