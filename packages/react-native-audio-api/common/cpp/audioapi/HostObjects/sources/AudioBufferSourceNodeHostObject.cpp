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
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, onloopended));

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

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, onloopended) {
  audioBufferSourceNode_->assignOnLoopEndedCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_HOST_FUNCTION_IMPL(AudioBufferSourceNodeHostObject, start) {
  hasBeenStarted_ = true;
  acquireBufferContent();

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

void AudioBufferSourceNodeHostObject::acquireBufferContent() {
  if (assignedBufferHostObject_ != nullptr) {
    assignedBufferHostObject_->detachReturnedChannelData();
  }

  auto buffers = prepareNodeBuffers(assignedBufferHostObject_);
  auto event =
      [handle = node_->handle, node = audioBufferSourceNode_, buffers](BaseAudioContext &) {
        node->setBuffer(buffers.nodeBuffer, buffers.audioBuffer);
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

    setBuffer(bufferHostObject);
  }

  return jsi::Value::undefined();
}

AudioBufferSourceNodeHostObject::NodeBuffers AudioBufferSourceNodeHostObject::prepareNodeBuffers(
    const std::shared_ptr<AudioBufferHostObject> &bufferHostObject) {
  NodeBuffers buffers;

  if (bufferHostObject == nullptr) {
    buffers.nodeBuffer = nullptr;
    buffers.audioBuffer = std::make_shared<DSPAudioBuffer>(
        RENDER_QUANTUM_SIZE,
        AudioBufferSourceOptions::kDefaultOutputChannelNumber,
        audioBufferSourceNode_->getContextSampleRate());
    return buffers;
  }

  const auto &buffer = bufferHostObject->audioBuffer_;
  if (pitchCorrection_) {
    initStretch(static_cast<int>(buffer->getNumberOfChannels()), buffer->getSampleRate());
  }

  // The node reads the JS-facing storage directly; from now on JS writes to it go
  // copy-on-write, so nothing can change the samples under the node.
  buffers.nodeBuffer = bufferHostObject->shareForPlayback();

  buffers.audioBuffer = std::make_shared<DSPAudioBuffer>(
      RENDER_QUANTUM_SIZE,
      buffers.nodeBuffer->getNumberOfChannels(),
      audioBufferSourceNode_->getContextSampleRate());
  return buffers;
}

void AudioBufferSourceNodeHostObject::setBuffer(
    const std::shared_ptr<AudioBufferHostObject> &bufferHostObject) {
  assignedBufferHostObject_ = bufferHostObject;

  // Publish the new output width on the host thread before renegotiation so
  // MAX / CLAMPED_MAX downstream nodes see it immediately
  const size_t newOutputChannelNumber = bufferHostObject == nullptr
      ? AudioBufferSourceOptions::kDefaultOutputChannelNumber
      : bufferHostObject->audioBuffer_->getNumberOfChannels();
  if (newOutputChannelNumber != audioBufferSourceNode_->getOutputChannelNumber()) {
    audioBufferSourceNode_->setOutputChannelNumber(newOutputChannelNumber);
    renegotiate();
  }

  // Per Web Audio, assigning a buffer to an already-started source acquires its
  // content right away, because start() had nothing to acquire back then.
  if (hasBeenStarted_) {
    acquireBufferContent();
  }
}

} // namespace audioapi
