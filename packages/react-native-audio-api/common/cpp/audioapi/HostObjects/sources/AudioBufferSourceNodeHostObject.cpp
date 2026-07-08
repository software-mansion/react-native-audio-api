#include <audioapi/HostObjects/sources/AudioBufferSourceNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.h>
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

JSI_HOST_FUNCTION_IMPL(AudioBufferSourceNodeHostObject, setBuffer) {
  if (args[0].isNull()) {
    setBuffer(nullptr);
  } else {
    auto bufferHostObject = args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
    thisValue.asObject(runtime).setExternalMemoryPressure(
        runtime, getMemoryPressure() + bufferHostObject->getSizeInBytes());

    setBuffer(bufferHostObject->audioBuffer_);
  }

  return jsi::Value::undefined();
}

void AudioBufferSourceNodeHostObject::setBuffer(const std::shared_ptr<AudioBuffer> &buffer) {
  // TODO: add optimized memory management for buffer changes, e.g.
  //  when the same buffer is reused across threads and
  // buffer modification is not allowed on JS thread
  auto handle = node_->handle;

  std::shared_ptr<AudioBuffer> copiedBuffer;
  std::shared_ptr<DSPAudioBuffer> audioBuffer;

  if (buffer == nullptr) {
    copiedBuffer = nullptr;
    audioBuffer = std::make_shared<DSPAudioBuffer>(
        RENDER_QUANTUM_SIZE, 1, audioBufferSourceNode_->getContextSampleRate());
  } else {
    if (pitchCorrection_) {
      initStretch(static_cast<int>(buffer->getNumberOfChannels()), buffer->getSampleRate());
      auto extraTailFrames =
          static_cast<size_t>((inputLatency_ + outputLatency_) * buffer->getSampleRate());
      size_t totalSize = buffer->getSize() + extraTailFrames;
      copiedBuffer = std::make_shared<AudioBuffer>(
          totalSize, buffer->getNumberOfChannels(), buffer->getSampleRate());
      copiedBuffer->copy(*buffer, 0, 0, buffer->getSize());
      copiedBuffer->zero(buffer->getSize(), extraTailFrames);
    } else {
      copiedBuffer = std::make_shared<AudioBuffer>(*buffer);
    }

    audioBuffer = std::make_shared<DSPAudioBuffer>(
        RENDER_QUANTUM_SIZE,
        copiedBuffer->getNumberOfChannels(),
        audioBufferSourceNode_->getContextSampleRate());
  }

  auto event =
      [handle, node = audioBufferSourceNode_, copiedBuffer, audioBuffer](BaseAudioContext &) {
        node->setBuffer(copiedBuffer, audioBuffer);
      };
  audioBufferSourceNode_->scheduleAudioEvent(std::move(event));
}

} // namespace audioapi
