#include <audioapi/HostObjects/sources/AudioBufferQueueSourceNodeHostObject.h>

#include <audioapi/HostObjects/TypedAudioNodePtr.hpp>
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

  addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioBufferQueueSourceNodeHostObject, onbufferended));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, start),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, enqueueBuffer),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, dequeueBuffer),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, clearBuffers),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, pause),
      JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, resume));
}

AudioBufferQueueSourceNodeHostObject::~AudioBufferQueueSourceNodeHostObject() {
  bufferQueueSourceNode_->assignOnBufferEndedCallbackId(0);
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferQueueSourceNodeHostObject, onbufferended) {
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

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, resume) {
  auto handle = node_->handle;
  auto event =
      [handle, node = bufferQueueSourceNode_, when = args[0].getNumber()](BaseAudioContext &) {
        node->resume(when);
      };
  bufferQueueSourceNode_->scheduleAudioEvent(std::move(event));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferQueueSourceNodeHostObject, enqueueBuffer) {
  auto audioBufferHostObject =
      args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
  const auto &samples = audioBufferHostObject->audioBuffer_;
  const auto bufferChannelNumber = samples->getNumberOfChannels();

  // The first buffer fixes the width the queue plays at.
  if (outputChannelNumber_ == 0) {
    outputChannelNumber_ = bufferChannelNumber;
  }

  std::shared_ptr<AudioBuffer> content;

  if (bufferChannelNumber == outputChannelNumber_) {
    // Played from the JS buffer's own storage: its live getChannelData() views are cut off
    // and JS writes go copy-on-write from now on, so nothing can change the samples under
    // the node.
    audioBufferHostObject->detachReturnedChannelData();
    content = audioBufferHostObject->shareForPlayback();
  } else {
    // A buffer of another width is mixed into a private copy of the queue's width.
    content = std::make_shared<AudioBuffer>(
        samples->getSize(), static_cast<int>(outputChannelNumber_), samples->getSampleRate());
    content->sum(*samples);
  }

  std::shared_ptr<AudioBuffer> tailBuffer = nullptr;

  if (pitchCorrection_ && !stretchHasBeenInit_) {
    initStretch(static_cast<int>(outputChannelNumber_), samples->getSampleRate());
    auto extraTailFrames =
        static_cast<size_t>((inputLatency_ + outputLatency_) * samples->getSampleRate());
    tailBuffer = std::make_shared<AudioBuffer>(
        extraTailFrames, static_cast<int>(outputChannelNumber_), samples->getSampleRate());
    tailBuffer->zero();
  }

  auto event = [handle = node_->handle,
                node = bufferQueueSourceNode_,
                content,
                bufferId = bufferId_,
                tailBuffer](BaseAudioContext &) {
    node->resizeOutputBuffer(content->getNumberOfChannels());
    node->enqueueBuffer(content, bufferId, tailBuffer);
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
