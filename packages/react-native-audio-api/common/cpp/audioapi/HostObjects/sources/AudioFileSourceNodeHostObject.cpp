#include <audioapi/HostObjects/sources/AudioFileSourceNodeHostObject.h>

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
    : AudioScheduledSourceNodeHostObject(context->createFileSource(options), options),
      loop_(options.loop),
      duration_(std::static_pointer_cast<AudioFileSourceNode>(node_)->getDuration()),
      volume_(options.volume) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, volume),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, loop),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, currentTime),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, duration),
      JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, routedThroughMediaElement));
  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, onPositionChanged),
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, volume),
      JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, loop));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioFileSourceNodeHostObject, pause),
      JSI_EXPORT_FUNCTION(AudioFileSourceNodeHostObject, start),
      JSI_EXPORT_FUNCTION(AudioFileSourceNodeHostObject, seekToTime));
}

AudioFileSourceNodeHostObject::~AudioFileSourceNodeHostObject() {
  auto node = std::static_pointer_cast<AudioFileSourceNode>(node_);
  node->assignOnPositionChangedCallbackId(0);
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, volume) {
  return {volume_};
}

JSI_PROPERTY_SETTER_IMPL(AudioFileSourceNodeHostObject, volume) {
  auto node = std::static_pointer_cast<AudioFileSourceNode>(node_);
  volume_ = static_cast<float>(value.getNumber());
  auto event = [node, volume = this->volume_](BaseAudioContext &ctx) {
    node->setVolume(volume);
  };
  node->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, loop) {
  return {loop_};
}

JSI_PROPERTY_SETTER_IMPL(AudioFileSourceNodeHostObject, loop) {
  auto node = std::static_pointer_cast<AudioFileSourceNode>(node_);
  loop_ = value.getBool();
  auto event = [node, loop = this->loop_](BaseAudioContext &ctx) {
    node->setLoop(loop);
  };
  node->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, currentTime) {
  auto node = std::static_pointer_cast<AudioFileSourceNode>(node_);
  return {node->getCurrentTime()};
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, duration) {
  return {duration_};
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, routedThroughMediaElement) {
  auto node = std::static_pointer_cast<AudioFileSourceNode>(node_);
  return {node->isRoutedThroughMediaElement()};
}

JSI_HOST_FUNCTION_IMPL(AudioFileSourceNodeHostObject, pause) {
  auto audioFileSourceNode = std::static_pointer_cast<AudioFileSourceNode>(node_);
  auto event = [audioFileSourceNode](BaseAudioContext &ctx) {
    audioFileSourceNode->pause();
  };
  audioFileSourceNode->scheduleAudioEvent(std::move(event));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioFileSourceNodeHostObject, seekToTime) {
  auto audioFileSourceNode = std::static_pointer_cast<AudioFileSourceNode>(node_);
  if (count < 1 || !args[0].isNumber()) {
    return jsi::Value::undefined();
  }
  const double t = args[0].getNumber();

  auto event = [audioFileSourceNode, t](BaseAudioContext &) {
    audioFileSourceNode->seekToTime(t);
  };
  audioFileSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_PROPERTY_SETTER_IMPL(AudioFileSourceNodeHostObject, onPositionChanged) {
  auto sourceNode = std::static_pointer_cast<AudioFileSourceNode>(node_);
  sourceNode->assignOnPositionChangedCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

} // namespace audioapi
