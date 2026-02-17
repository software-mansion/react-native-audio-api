#include <audioapi/HostObjects/sources/AudioBufferQueueSourceNodeHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <memory>

namespace audioapi {

AudioBufferQueueSourceNodeHostObject::AudioBufferQueueSourceNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const BaseAudioBufferSourceOptions &options)
    : AudioBufferBaseSourceNodeHostObject(context->createBufferQueueSource(options), options) {
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
  auto when = args[0].getNumber();

  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  if (!args[1].isNumber()) {
    audioBufferQueueSourceNode->start(when);
  } else {
    auto offset = args[1].getNumber();
    audioBufferQueueSourceNode->start(when, offset);
  }

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, pause) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  audioBufferQueueSourceNode->pause();

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, enqueueBuffer) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto audioBufferHostObject =
      args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);

  auto bufferId = audioBufferQueueSourceNode->enqueueBuffer(audioBufferHostObject->audioBuffer_);

  return jsi::String::createFromUtf8(runtime, bufferId);
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, dequeueBuffer) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto bufferId = static_cast<size_t>(args[0].getNumber());

  audioBufferQueueSourceNode->dequeueBuffer(bufferId);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, clearBuffers) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  audioBufferQueueSourceNode->clearBuffers();

  return jsi::Value::undefined();
}

} // namespace audioapi
