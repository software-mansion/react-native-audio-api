#include <audioapi/HostObjects/sources/AudioFileSourceNodeHostObject.h>

#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/types/NodeOptions.h>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AudioFileSourceNodeHostObject::AudioFileSourceNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    AudioFileSourceOptions &options)
    : AudioScheduledSourceNodeHostObject(
          context->getGraph(),
          std::make_unique<AudioFileSourceNode>(context, options),
          options),
      audioFileSourceNode_(typedAudioNode<AudioFileSourceNode>(node_)),
      loop_(options.loop),
      duration_(audioFileSourceNode_->getDuration()),
      volume_(options.volume),
      playbackRate_(options.playbackRate),
      preservesPitch_(options.preservesPitch) {
  if (audioFileSourceNode_->isHlsStreaming()) {
    playbackRate_ = 1.0f;
  }

  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, volume),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, playbackRate),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, preservesPitch),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, loop),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, currentTime),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, duration),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, routedThroughMediaElement));
  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, onPositionChanged),
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, volume),
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, playbackRate),
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, preservesPitch),
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, loop));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioFileSourceNodeHostObject, pause),
      JSI_EXPORT_FUNCTION(AudioFileSourceNodeHostObject, start),
      JSI_EXPORT_FUNCTION(AudioFileSourceNodeHostObject, seekToTime));
}

AudioFileSourceNodeHostObject::~AudioFileSourceNodeHostObject() {
  audioFileSourceNode_->assignOnPositionChangedCallbackId(0);
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

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, playbackRate) {
  return {playbackRate_};
}

JSI_PROPERTY_SETTER_IMPL(AudioFileSourceNodeHostObject, playbackRate) {
  if (audioFileSourceNode_->isHlsStreaming()) {
    return;
  }

  playbackRate_ = static_cast<float>(value.getNumber());
  auto event = [node = audioFileSourceNode_,
                playbackRate = this->playbackRate_](BaseAudioContext &) {
    node->setPlaybackRate(playbackRate);
  };
  audioFileSourceNode_->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, preservesPitch) {
  return {preservesPitch_};
}

JSI_PROPERTY_SETTER_IMPL(AudioFileSourceNodeHostObject, preservesPitch) {
  preservesPitch_ = value.getBool();
  auto event = [node = audioFileSourceNode_,
                preservesPitch = this->preservesPitch_](BaseAudioContext &) {
    node->setPreservesPitch(preservesPitch);
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
  audioFileSourceNode_->assignOnPositionChangedCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

} // namespace audioapi
