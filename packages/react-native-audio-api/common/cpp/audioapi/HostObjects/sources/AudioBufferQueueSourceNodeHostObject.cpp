#include <audioapi/HostObjects/sources/AudioBufferQueueSourceNodeHostObject.h>

#include <audioapi/HostObjects/TypedAudioNodePtr.h>
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
    : AudioBufferBaseSourceNodeHostObject(
          context->getGraph(),
          std::make_unique<AudioBufferQueueSourceNode>(context, options),
          options),
      bufferQueueSourceNode_(typedAudioNode<AudioBufferQueueSourceNode>(node_)) {
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
  bufferQueueSourceNode_->assignOnBufferEndedCallbackId(0);
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferQueueSourceNodeHostObject, onBufferEnded) {
  bufferQueueSourceNode_->assignOnBufferEndedCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, start) {
  auto handle = node_->handle;
  auto event = [handle,
                node = bufferQueueSourceNode_,
                when = args[0].getNumber(),
                offset = args[1].getNumber()](BaseAudioContext &) {
    node->start(when, offset);
  };
  bufferQueueSourceNode_->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, pause) {
  auto handle = node_->handle;
  auto event = [handle, node = bufferQueueSourceNode_](BaseAudioContext &) {
    node->pause();
  };
  bufferQueueSourceNode_->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, enqueueBuffer) {
  auto audioBufferHostObject =
      args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
  // TODO: add optimized memory management for buffer changes, e.g.
  // when the same buffer is reused across threads and
  // buffer modification is not allowed on JS thread

  auto swapBuffer = false; // whether to swap internal node buffer with the new buffer
  if (!channelCountSet_) {
    channelCount_ = static_cast<int>(audioBufferHostObject->audioBuffer_->getNumberOfChannels());
    channelCountSet_ = true;
    swapBuffer = true;
  }

  // first buffer defines channel count, rest of them is mixed to channel count of the first buffer
  auto copiedBuffer = std::make_shared<AudioBuffer>(
      audioBufferHostObject->audioBuffer_->getSize(),
      channelCount_,
      audioBufferHostObject->audioBuffer_->getSampleRate());

  copiedBuffer->sum(*audioBufferHostObject->audioBuffer_);

  std::shared_ptr<AudioBuffer> tailBuffer = nullptr;

  if (pitchCorrection_ && !stretchHasBeenInit_) {
    initStretch(
        static_cast<int>(copiedBuffer->getNumberOfChannels()), copiedBuffer->getSampleRate());
    auto extraTailFrames =
        static_cast<size_t>((inputLatency_ + outputLatency_) * copiedBuffer->getSampleRate());
    tailBuffer = std::make_shared<AudioBuffer>(
        extraTailFrames, copiedBuffer->getNumberOfChannels(), copiedBuffer->getSampleRate());
    tailBuffer->zero();
    stretchHasBeenInit_ = true;
  }

  auto event = [node = bufferQueueSourceNode_,
                copiedBuffer,
                bufferId = bufferId_,
                tailBuffer,
                swapBuffer,
                channelCount = channelCount_](BaseAudioContext &) {
    if (swapBuffer) {
      node->setChannelCount(static_cast<int>(channelCount));
    }
    node->enqueueBuffer(copiedBuffer, bufferId, tailBuffer);
  };
  bufferQueueSourceNode_->scheduleAudioEvent(std::move(event));

  return jsi::String::createFromUtf8(runtime, std::to_string(bufferId_++));
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, dequeueBuffer) {
  auto handle = node_->handle;
  auto event = [handle,
                node = bufferQueueSourceNode_,
                bufferId = static_cast<size_t>(args[0].getNumber())](BaseAudioContext &) {
    node->dequeueBuffer(bufferId);
  };
  bufferQueueSourceNode_->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, clearBuffers) {
  auto handle = node_->handle;
  auto event = [handle, node = bufferQueueSourceNode_](BaseAudioContext &) {
    node->clearBuffers();
  };
  bufferQueueSourceNode_->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

} // namespace audioapi
