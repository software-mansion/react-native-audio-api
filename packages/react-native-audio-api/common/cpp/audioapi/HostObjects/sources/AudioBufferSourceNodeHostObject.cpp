#include <audioapi/HostObjects/sources/AudioBufferSourceNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.hpp>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <utility>

namespace audioapi {

AudioBufferSourceNodeHostObject::AudioBufferSourceNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioBufferSourceOptions &options)
    : AudioBufferBaseSourceNodeHostObject(
          context->getGraph(),
          std::make_unique<AudioBufferSourceNode>(context, options),
          options),
      audioBufferSourceNode_(typedAudioNode<AudioBufferSourceNode>(node_)),
      loop_(options.loop),
      loopSkip_(options.loopSkip),
      loopStart_(options.loopStart),
      loopEnd_(options.loopEnd) {
  if (options.buffer != nullptr) {
    setBuffer(options.buffer);
  }

  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loop),
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopSkip),
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopStart),
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopEnd));

  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loop),
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loopSkip),
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loopStart),
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loopEnd),
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, onLoopEnded));

  // start method is overridden in this class
  functions_->erase("start");

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioBufferSourceNodeHostObject, start),
      JSI_EXPORT_FUNCTION(AudioBufferSourceNodeHostObject, setBuffer));
}

AudioBufferSourceNodeHostObject::~AudioBufferSourceNodeHostObject() {
  // When JSI object is garbage collected (together with the eventual callback),
  // underlying source node might still be active and try to call the
  // non-existing callback.
  audioBufferSourceNode_->assignOnLoopEndedCallbackId(0);
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferSourceNodeHostObject, loop) {
  return {loop_};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferSourceNodeHostObject, loopSkip) {
  return {loopSkip_};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferSourceNodeHostObject, loopStart) {
  return {loopStart_};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferSourceNodeHostObject, loopEnd) {
  return {loopEnd_};
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, loop) {
  auto handle = node_->handle;
  auto loop = value.getBool();

  auto event = [handle, node = audioBufferSourceNode_, loop](BaseAudioContext &) {
    node->setLoop(loop);
  };

  audioBufferSourceNode_->scheduleAudioEvent(std::move(event));
  loop_ = loop;
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, loopSkip) {
  auto handle = node_->handle;
  auto loopSkip = value.getBool();

  auto event = [handle, node = audioBufferSourceNode_, loopSkip](BaseAudioContext &) {
    node->setLoopSkip(loopSkip);
  };

  audioBufferSourceNode_->scheduleAudioEvent(std::move(event));
  loopSkip_ = loopSkip;
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, loopStart) {
  auto handle = node_->handle;
  auto loopStart = value.getNumber();

  auto event = [handle, node = audioBufferSourceNode_, loopStart](BaseAudioContext &) {
    node->setLoopStart(loopStart);
  };

  audioBufferSourceNode_->scheduleAudioEvent(std::move(event));
  loopStart_ = loopStart;
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, loopEnd) {
  auto handle = node_->handle;
  auto loopEnd = value.getNumber();

  auto event = [handle, node = audioBufferSourceNode_, loopEnd](BaseAudioContext &) {
    node->setLoopEnd(loopEnd);
  };

  audioBufferSourceNode_->scheduleAudioEvent(std::move(event));
  loopEnd_ = loopEnd;
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, onLoopEnded) {
  audioBufferSourceNode_->assignOnLoopEndedCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_HOST_FUNCTION_IMPL(AudioBufferSourceNodeHostObject, start) {
  hasBeenStarted_ = true;
  acquireBufferContent(runtime);

  auto handle = node_->handle;
  auto event = [handle,
                node = audioBufferSourceNode_,
                when = args[0].getNumber(),
                offset = args[1].getNumber(),
                duration = args[2].isUndefined() ? -1 : args[2].getNumber()](BaseAudioContext &) {
    node->start(when, offset, duration);
  };
  audioBufferSourceNode_->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

void AudioBufferSourceNodeHostObject::acquireBufferContent(jsi::Runtime &runtime) {
  if (bufferHostObject_ == nullptr || !bufferHostObject_->hasReturnedChannelData()) {
    return;
  }

  bufferHostObject_->detachReturnedChannelData(runtime);
  // The copy handed to the node in setBuffer() predates any writes made through those
  // views since, so hand it the post-write content that has now been fenced off.
  auto buffers = prepareNodeBuffers(bufferHostObject_->audioBuffer_, bufferHostObject_);
  auto event =
      [handle = node_->handle, node = audioBufferSourceNode_, buffers](BaseAudioContext &) {
        node->replaceBufferContent(buffers.copiedBuffer, buffers.audioBuffer);
      };
  audioBufferSourceNode_->scheduleAudioEvent(std::move(event));
}

JSI_HOST_FUNCTION_IMPL(AudioBufferSourceNodeHostObject, setBuffer) {
  if (args[0].isNull()) {
    setBuffer(nullptr);
  } else {
    auto bufferHostObject = args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
    thisValue.asObject(runtime).setExternalMemoryPressure(
        runtime, getMemoryPressure() + bufferHostObject->getSizeInBytes());

    setBuffer(bufferHostObject->audioBuffer_, bufferHostObject);
  }

  // Per Web Audio, assigning a buffer to an already-started source acquires its
  // content right away, because start() had nothing to acquire back then.
  if (hasBeenStarted_) {
    acquireBufferContent(runtime);
  }

  return jsi::Value::undefined();
}

AudioBufferSourceNodeHostObject::NodeBuffers AudioBufferSourceNodeHostObject::prepareNodeBuffers(
    const std::shared_ptr<AudioBuffer> &buffer,
    const std::shared_ptr<AudioBufferHostObject> &bufferHostObject) {
  // TODO: add optimized memory management for buffer changes, e.g.
  //  when the same buffer is reused across threads and
  // buffer modification is not allowed on JS thread
  NodeBuffers buffers;

  if (buffer == nullptr) {
    buffers.copiedBuffer = nullptr;
    buffers.audioBuffer = std::make_shared<DSPAudioBuffer>(
        RENDER_QUANTUM_SIZE,
        AudioBufferSourceOptions::kDefaultChannelCount,
        audioBufferSourceNode_->getContextSampleRate());
    return buffers;
  }

  if (pitchCorrection_) {
    initStretch(static_cast<int>(buffer->getNumberOfChannels()), buffer->getSampleRate());
    auto extraTailFrames =
        static_cast<size_t>((inputLatency_ + outputLatency_) * buffer->getSampleRate());
    size_t totalSize = buffer->getSize() + extraTailFrames;
    buffers.copiedBuffer = std::make_shared<AudioBuffer>(
        totalSize, buffer->getNumberOfChannels(), buffer->getSampleRate());
    buffers.copiedBuffer->copy(*buffer, 0, 0, buffer->getSize());
    buffers.copiedBuffer->zero(buffer->getSize(), extraTailFrames);
  } else if (bufferHostObject != nullptr) {
    // Reuse a cached copy across repeated `.buffer = x` reassignments of the same
    // JS-visible buffer (e.g. seeking, which recreates the source node but keeps
    // reusing the already-decoded buffer) instead of deep-copying every time.
    // See https://github.com/software-mansion/react-native-audio-api/issues/1263.
    buffers.copiedBuffer = bufferHostObject->getOrCreateImmutableCopy();
  } else {
    buffers.copiedBuffer = std::make_shared<AudioBuffer>(*buffer);
  }

  buffers.audioBuffer = std::make_shared<DSPAudioBuffer>(
      RENDER_QUANTUM_SIZE,
      buffers.copiedBuffer->getNumberOfChannels(),
      audioBufferSourceNode_->getContextSampleRate());
  return buffers;
}

void AudioBufferSourceNodeHostObject::setBuffer(
    const std::shared_ptr<AudioBuffer> &buffer,
    const std::shared_ptr<AudioBufferHostObject> &bufferHostObject) {
  bufferHostObject_ = bufferHostObject;
  auto buffers = prepareNodeBuffers(buffer, bufferHostObject);

  // Update channelCount on the host thread before renegotiation so MAX /
  // CLAMPED_MAX downstream nodes see the new width immediately.
  const size_t newChannelCount = buffer == nullptr ? AudioBufferSourceOptions::kDefaultChannelCount
                                                   : buffer->getNumberOfChannels();
  updateChannelCount(newChannelCount);

  auto event =
      [handle = node_->handle, node = audioBufferSourceNode_, buffers](BaseAudioContext &) {
        node->setBuffer(buffers.copiedBuffer, buffers.audioBuffer);
      };
  audioBufferSourceNode_->scheduleAudioEvent(std::move(event));
}

} // namespace audioapi
