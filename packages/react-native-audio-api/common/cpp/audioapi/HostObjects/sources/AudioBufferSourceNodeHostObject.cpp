#include <audioapi/HostObjects/sources/AudioBufferSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/utils/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>

#include <memory>
#include <utility>

namespace audioapi {

AudioBufferSourceNodeHostObject::AudioBufferSourceNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioBufferSourceOptions &options)
    : AudioBufferBaseSourceNodeHostObject(context->createBufferSource(options)),
    loop_(options.loop),
    loopStart_(options.loopStart),
    loopEnd_(options.loopEnd),
    buffer_(std::make_shared<AudioBufferHostObject>(options.buffer)) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loop),
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopSkip),
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, buffer),
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

JSI_PROPERTY_GETTER_IMPL(AudioBufferSourceNodeHostObject, buffer) {
  if (!buffer_) {
    return jsi::Value::null();
  }

  auto jsiObject = jsi::Object::createFromHostObject(runtime, buffer_);
  jsiObject.setExternalMemoryPressure(runtime, buffer_->getSizeInBytes() + 16);
  return jsiObject;
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

  auto event = [
          audioBufferSourceNode,
          when = args[0].getNumber(),
          offset = args[1].getNumber(),
          duration = args[2].isUndefined() ? -1 : args[2].getNumber()
      ](BaseAudioContext &) {
    audioBufferSourceNode->start(when, offset, duration);
  };

  audioBufferSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferSourceNodeHostObject, setBuffer) {
  auto audioBufferSourceNode = std::static_pointer_cast<AudioBufferSourceNode>(node_);

  auto bufferHostObject = args[0].isNull() ? std::shared_ptr<AudioBufferHostObject>(nullptr) :
                        args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);


  std::shared_ptr<AudioBuffer> bufferCopy = nullptr;

  if (bufferHostObject != nullptr) {
    thisValue.asObject(runtime).setExternalMemoryPressure(
            runtime, bufferHostObject->getSizeInBytes() + 16);

    bufferCopy = std::make_shared<AudioBuffer>(*bufferHostObject->audioBuffer_);
  }

  auto event = [
        audioBufferSourceNode,
        buffer = bufferCopy
  ](BaseAudioContext &) {
    audioBufferSourceNode->setBuffer(buffer);
  };

  audioBufferSourceNode->scheduleAudioEvent(std::move(event));
  buffer_ = bufferHostObject;
  loopEnd_ = bufferHostObject ? bufferHostObject->audioBuffer_->getDuration() : 0.0;

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

} // namespace audioapi
