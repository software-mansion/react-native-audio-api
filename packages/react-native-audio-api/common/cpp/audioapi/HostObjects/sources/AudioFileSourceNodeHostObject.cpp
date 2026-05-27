#include <audioapi/HostObjects/sources/AudioFileSourceNodeHostObject.h>

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
      loop_(options.loop),
      duration_(
          static_cast<AudioFileSourceNode *>(node_->handle->audioNode->asAudioNode())
              ->getDuration()),
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
  auto handle = node_->handle;
  auto *node = static_cast<AudioFileSourceNode *>(handle->audioNode->asAudioNode());
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
  auto handle = node_->handle;
  auto *node = static_cast<AudioFileSourceNode *>(handle->audioNode->asAudioNode());
  loop_ = value.getBool();
  auto event = [node, loop = this->loop_](BaseAudioContext &ctx) {
    node->setLoop(loop);
  };
  node->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, currentTime) {
  auto handle = node_->handle;
  auto *node = static_cast<AudioFileSourceNode *>(handle->audioNode->asAudioNode());
  return {node->getCurrentTime()};
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, duration) {
  return {duration_};
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, routedThroughMediaElement) {
  auto *node = getAudioFileSourceNode();
  return {node->isRoutedThroughMediaElement()};
}

JSI_HOST_FUNCTION_IMPL(AudioFileSourceNodeHostObject, pause) {
  auto handle = node_->handle;
  auto *audioFileSourceNode = static_cast<AudioFileSourceNode *>(handle->audioNode->asAudioNode());
  auto event = [audioFileSourceNode](BaseAudioContext &ctx) {
    audioFileSourceNode->pause();
  };
  audioFileSourceNode->scheduleAudioEvent(std::move(event));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioFileSourceNodeHostObject, seekToTime) {
  auto handle = node_->handle;
  auto *audioFileSourceNode = static_cast<AudioFileSourceNode *>(handle->audioNode->asAudioNode());
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
  auto callbackId = std::stoull(value.getString(runtime).utf8(runtime));
  setOnPositionChangedCallbackId(callbackId);
}

void AudioFileSourceNodeHostObject::setOnPositionChangedCallbackId(uint64_t callbackId) {
  auto handle = node_->handle;
  auto *sourceNode = static_cast<AudioFileSourceNode *>(handle->audioNode->asAudioNode());

  auto event = [sourceNode, callbackId](BaseAudioContext &) {
    sourceNode->setOnPositionChangedCallbackId(callbackId);
  };

  sourceNode->unregisterOnPositionChangedCallback(onPositionChangedCallbackId_);
  sourceNode->scheduleAudioEvent(std::move(event));
  onPositionChangedCallbackId_ = callbackId;
}

void AudioFileSourceNodeHostObject::setOnEndedCallbackId(uint64_t callbackId) {
  auto handle = node_->handle;
  auto *sourceNode = static_cast<AudioFileSourceNode *>(handle->audioNode->asAudioNode());

  auto event = [sourceNode, callbackId](BaseAudioContext &) {
    sourceNode->setOnEndedCallbackId(callbackId);
  };

  sourceNode->unregisterOnEndedCallback(onEndedCallbackId_);
  sourceNode->scheduleAudioEvent(std::move(event));
  onEndedCallbackId_ = callbackId;
}

} // namespace audioapi
