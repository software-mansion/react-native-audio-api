#include <audioapi/HostObjects/sources/AudioBufferQueueSourceNodeHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/types/NodeOptions.h>
#include <memory>
#include <string>
#include <utility>

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
  // When JSI object is garbage collected (together with the eventual callback),
  // underlying source node might still be active and try to call the
  // non-existing callback.
  setOnBufferEndedCallbackId(0);
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferQueueSourceNodeHostObject, onBufferEnded) {
  auto callbackId = std::stoull(value.getString(runtime).utf8(runtime));
  setOnBufferEndedCallbackId(callbackId);
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, start) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto event = [audioBufferQueueSourceNode,
                when = args[0].getNumber(),
                offset = args[1].getNumber()](BaseAudioContext &) {
    audioBufferQueueSourceNode->start(when, offset);
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
  // TODO: add optimized memory management for buffer changes, e.g.
  //  when the same buffer is reused across threads and
  // buffer modification is not allowed on JS thread
  auto copiedBuffer = std::make_shared<AudioBuffer>(*audioBufferHostObject->audioBuffer_);
  std::shared_ptr<AudioBuffer> tailBuffer = nullptr;

  if (pitchCorrection_ && !stretchHasBeenInit_) {
    initStretch(copiedBuffer->getNumberOfChannels(), copiedBuffer->getSampleRate());
    int extraTailFrames =
        static_cast<size_t>((inputLatency_ + outputLatency_) * copiedBuffer->getSampleRate());
    tailBuffer = std::make_shared<AudioBuffer>(
        copiedBuffer->getNumberOfChannels(), extraTailFrames, copiedBuffer->getSampleRate());
    tailBuffer->zero();
    stretchHasBeenInit_ = true;
  }

  auto event = [audioBufferQueueSourceNode, copiedBuffer, bufferId = bufferId_, tailBuffer](
                   BaseAudioContext &) {
    audioBufferQueueSourceNode->enqueueBuffer(copiedBuffer, bufferId, tailBuffer);
  };
  audioBufferQueueSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::String::createFromUtf8(runtime, std::to_string(bufferId_++));
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, dequeueBuffer) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto event = [audioBufferQueueSourceNode,
                bufferId = static_cast<size_t>(args[0].getNumber())](BaseAudioContext &) {
    audioBufferQueueSourceNode->dequeueBuffer(bufferId);
  };
  audioBufferQueueSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, clearBuffers) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto event = [audioBufferQueueSourceNode](BaseAudioContext &) {
    audioBufferQueueSourceNode->clearBuffers();
  };
  audioBufferQueueSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

void AudioBufferQueueSourceNodeHostObject::setOnBufferEndedCallbackId(uint64_t callbackId) {
  auto audioBufferQueueSourceNode = std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

  auto event = [audioBufferQueueSourceNode, callbackId](BaseAudioContext &) {
    audioBufferQueueSourceNode->setOnBufferEndedCallbackId(callbackId);
  };

  audioBufferQueueSourceNode->unregisterOnBufferEndedCallback(onBufferEndedCallbackId_);
  audioBufferQueueSourceNode->scheduleAudioEvent(std::move(event));
  onBufferEndedCallbackId_ = callbackId;
}

} // namespace audioapi
