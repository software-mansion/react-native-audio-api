#include <audioapi/HostObjects/sources/AudioBufferSourceNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
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
    : AudioBufferBaseSourceNodeHostObject(context->createBufferSource(options), options),
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
  setOnLoopEndedCallbackId(0);
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
  auto audioBufferSourceNode = std::static_pointer_cast<AudioBufferSourceNode>(node_);
  auto loop = value.getBool();

  auto event = [audioBufferSourceNode, loop](BaseAudioContext &) {
    audioBufferSourceNode->setLoop(loop);
  };

  audioBufferSourceNode->scheduleAudioEvent(std::move(event));
  loop_ = loop;
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, loopSkip) {
  auto audioBufferSourceNode = std::static_pointer_cast<AudioBufferSourceNode>(node_);
  auto loopSkip = value.getBool();

  auto event = [audioBufferSourceNode, loopSkip](BaseAudioContext &) {
    audioBufferSourceNode->setLoopSkip(loopSkip);
  };

  audioBufferSourceNode->scheduleAudioEvent(std::move(event));
  loopSkip_ = loopSkip;
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, loopStart) {
  auto audioBufferSourceNode = std::static_pointer_cast<AudioBufferSourceNode>(node_);
  auto loopStart = value.getNumber();

  auto event = [audioBufferSourceNode, loopStart](BaseAudioContext &) {
    audioBufferSourceNode->setLoopStart(loopStart);
  };

  audioBufferSourceNode->scheduleAudioEvent(std::move(event));
  loopStart_ = loopStart;
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, loopEnd) {
  auto audioBufferSourceNode = std::static_pointer_cast<AudioBufferSourceNode>(node_);
  auto loopEnd = value.getNumber();

  auto event = [audioBufferSourceNode, loopEnd](BaseAudioContext &) {
    audioBufferSourceNode->setLoopEnd(loopEnd);
  };

  audioBufferSourceNode->scheduleAudioEvent(std::move(event));
  loopEnd_ = loopEnd;
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, onLoopEnded) {
  auto callbackId = std::stoull(value.getString(runtime).utf8(runtime));
  setOnLoopEndedCallbackId(callbackId);
}

JSI_HOST_FUNCTION_IMPL(AudioBufferSourceNodeHostObject, start) {
  auto audioBufferSourceNode = std::static_pointer_cast<AudioBufferSourceNode>(node_);

  auto event = [audioBufferSourceNode,
                when = args[0].getNumber(),
                offset = args[1].getNumber(),
                duration = args[2].isUndefined() ? -1 : args[2].getNumber()](BaseAudioContext &) {
    audioBufferSourceNode->start(when, offset, duration);
  };
  audioBufferSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferSourceNodeHostObject, setBuffer) {
  auto audioBufferSourceNode = std::static_pointer_cast<AudioBufferSourceNode>(node_);

  if (args[0].isNull()) {
    setBuffer(nullptr);
  } else {
    auto bufferHostObject = args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
    thisValue.asObject(runtime).setExternalMemoryPressure(
        runtime, bufferHostObject->getSizeInBytes());

    setBuffer(bufferHostObject->audioBuffer_);
  }

  return jsi::Value::undefined();
}

void AudioBufferSourceNodeHostObject::setOnLoopEndedCallbackId(uint64_t callbackId) {
  auto audioBufferSourceNode = std::static_pointer_cast<AudioBufferSourceNode>(node_);

  auto event = [audioBufferSourceNode, callbackId](BaseAudioContext &) {
    audioBufferSourceNode->setOnLoopEndedCallbackId(callbackId);
  };

  audioBufferSourceNode->unregisterOnLoopEndedCallback(onLoopEndedCallbackId_);
  audioBufferSourceNode->scheduleAudioEvent(std::move(event));
  onLoopEndedCallbackId_ = callbackId;
}

void AudioBufferSourceNodeHostObject::setBuffer(const std::shared_ptr<AudioBuffer> &buffer) {
  // TODO: add optimized memory management for buffer changes, e.g.
  //  when the same buffer is reused across threads and
  // buffer modification is not allowed on JS thread
  auto audioBufferSourceNode = std::static_pointer_cast<AudioBufferSourceNode>(node_);

  std::shared_ptr<AudioBuffer> copiedBuffer;
  std::shared_ptr<DSPAudioBuffer> audioBuffer;

  if (buffer == nullptr) {
    copiedBuffer = nullptr;
    audioBuffer = std::make_shared<DSPAudioBuffer>(
        RENDER_QUANTUM_SIZE, 1, audioBufferSourceNode->getContextSampleRate());
  } else {
    if (pitchCorrection_) {
      initStretch(static_cast<int>(buffer->getNumberOfChannels()), buffer->getSampleRate());
      int extraTailFrames =
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
        audioBufferSourceNode->getContextSampleRate());
  }

  auto event = [audioBufferSourceNode, copiedBuffer, audioBuffer](BaseAudioContext &) {
    audioBufferSourceNode->setBuffer(copiedBuffer, audioBuffer);
  };
  audioBufferSourceNode->scheduleAudioEvent(std::move(event));
}

} // namespace audioapi
