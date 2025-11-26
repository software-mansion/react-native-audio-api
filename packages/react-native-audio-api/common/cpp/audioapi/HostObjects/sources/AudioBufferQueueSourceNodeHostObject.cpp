#include <audioapi/HostObjects/sources/AudioBufferQueueSourceNodeHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <memory>

namespace audioapi {

AudioBufferQueueSourceNodeHostObject::AudioBufferQueueSourceNodeHostObject(
    const std::shared_ptr<AudioBufferQueueSourceNode> &node)
    : AudioBufferBaseSourceNodeHostObject(node) {
  functions_->erase("start");

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, start),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, enqueueBuffer),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, dequeueBuffer),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, clearBuffers),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, pause));
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
