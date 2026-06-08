#include <audioapi/HostObjects/sources/AudioFileSourceNodeHostObject.h>

#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/types/NodeOptions.h>
#include <memory>
#include <utility>

namespace audioapi {

AudioFileSourceNodeHostObject::AudioFileSourceNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioFileSourceOptions &options)
    : AudioScheduledSourceNodeHostObject(
          context->getGraph(),
          std::make_unique<AudioFileSourceNode>(context, options),
          options),
      audioFileSourceNode_(typedAudioNode<AudioFileSourceNode>(node_)),
      loop_(options.loop),
      duration_(audioFileSourceNode_->getDuration()),
      volume_(options.volume) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, volume),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, loop),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, currentTime),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, duration),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, routedThroughMediaElement));
  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, onPositionChanged),
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, onEnded),
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, volume),
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, loop));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioFileSourceNodeHostObject, pause),
      JSI_EXPORT_FUNCTION(AudioFileSourceNodeHostObject, start),
      JSI_EXPORT_FUNCTION(AudioFileSourceNodeHostObject, seekToTime));
}

AudioFileSourceNodeHostObject::~AudioFileSourceNodeHostObject() {
  setOnPositionChangedCallbackId(0);
  setOnEndedCallbackId(0);
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, volume) {
  return {volume_};
}

JSI_PROPERTY_SETTER_IMPL(AudioFileSourceNodeHostObject, volume) {
  volume_ = static_cast<float>(value.getNumber());
  auto event = [node = audioFileSourceNode_, volume = volume_](BaseAudioContext &) {
    node->setVolume(volume);
  };
  audioFileSourceNode_->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, loop) {
  return {loop_};
}

JSI_PROPERTY_SETTER_IMPL(AudioFileSourceNodeHostObject, loop) {
  loop_ = value.getBool();
  auto event = [node = audioFileSourceNode_, loop = loop_](BaseAudioContext &) {
    node->setLoop(loop);
  };
  audioFileSourceNode_->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, currentTime) {
  return {audioFileSourceNode_->getCurrentTime()};
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, duration) {
  return {duration_};
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, routedThroughMediaElement) {
  return {audioFileSourceNode_->isRoutedThroughMediaElement()};
}

JSI_HOST_FUNCTION_IMPL(AudioFileSourceNodeHostObject, pause) {
  auto event = [node = audioFileSourceNode_](BaseAudioContext &) {
    node->pause();
  };
  audioFileSourceNode_->scheduleAudioEvent(std::move(event));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioFileSourceNodeHostObject, seekToTime) {
  if (count < 1 || !args[0].isNumber()) {
    return jsi::Value::undefined();
  }
  const double t = args[0].getNumber();

  auto event = [node = audioFileSourceNode_, t](BaseAudioContext &) {
    node->seekToTime(t);
  };
  audioFileSourceNode_->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_PROPERTY_SETTER_IMPL(AudioFileSourceNodeHostObject, onPositionChanged) {
  auto callbackId = std::stoull(value.getString(runtime).utf8(runtime));
  setOnPositionChangedCallbackId(callbackId);
}

void AudioFileSourceNodeHostObject::setOnPositionChangedCallbackId(uint64_t callbackId) {
  auto event = [node = audioFileSourceNode_, callbackId](BaseAudioContext &) {
    node->setOnPositionChangedCallbackId(callbackId);
  };

  audioFileSourceNode_->unregisterOnPositionChangedCallback(onPositionChangedCallbackId_);
  audioFileSourceNode_->scheduleAudioEvent(std::move(event));
  onPositionChangedCallbackId_ = callbackId;
}

void AudioFileSourceNodeHostObject::setOnEndedCallbackId(uint64_t callbackId) {
  auto event = [node = audioFileSourceNode_, callbackId](BaseAudioContext &) {
    node->setOnEndedCallbackId(callbackId);
  };

  audioFileSourceNode_->unregisterOnEndedCallback(onEndedCallbackId_);
  audioFileSourceNode_->scheduleAudioEvent(std::move(event));
  onEndedCallbackId_ = callbackId;
}

} // namespace audioapi
