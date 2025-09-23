#include <audioapi/HostObjects/sources/AudioBufferQueueSourceNodeHostObject.h>

namespace audioapi {

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, pause) {
  auto audioBufferQueueSourceNode =
      std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  audioBufferQueueSourceNode->pause();

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, enqueueBuffer) {
  auto audioBufferQueueSourceNode =
      std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto audioBufferHostObject =
      args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);

  auto bufferId = audioBufferQueueSourceNode->enqueueBuffer(audioBufferHostObject->audioBuffer_);

  return jsi::String::createFromUtf8(runtime, bufferId);
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, dequeueBuffer) {
  auto audioBufferQueueSourceNode =
      std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto bufferId =
      args[0].getNumber();

  audioBufferQueueSourceNode->dequeueBuffer(bufferId);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, clearBuffers) {
  auto audioBufferQueueSourceNode =
      std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  audioBufferQueueSourceNode->clearBuffers();

  return jsi::Value::undefined();
}

} // namespace audioapi