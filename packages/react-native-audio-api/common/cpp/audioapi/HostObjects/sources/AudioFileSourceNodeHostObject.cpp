#include <audioapi/HostObjects/sources/AudioFileSourceNodeHostObject.h>

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/types/NodeOptions.h>
#include <memory>
#include <utility>

namespace audioapi {

AudioFileSourceNodeHostObject::AudioFileSourceNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioFileSourceOptions &options)
    : AudioScheduledSourceNodeHostObject(context->createFileSource(options), options) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(AudioFileSourceNodeHostObject, volume));
  addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioFileSourceNodeHostObject, volume));

  addFunctions(JSI_EXPORT_FUNCTION(AudioFileSourceNodeHostObject, pause));
}

JSI_PROPERTY_GETTER_IMPL(AudioFileSourceNodeHostObject, volume) {
  auto node = std::static_pointer_cast<AudioFileSourceNode>(node_);
  return jsi::Value(node->getVolume());
}

JSI_PROPERTY_SETTER_IMPL(AudioFileSourceNodeHostObject, volume) {
  auto node = std::static_pointer_cast<AudioFileSourceNode>(node_);
  node->setVolume(static_cast<float>(value.getNumber()));
}

JSI_HOST_FUNCTION_IMPL(AudioFileSourceNodeHostObject, pause) {
  auto audioFileSourceNode = std::static_pointer_cast<AudioFileSourceNode>(node_);

  auto event = [audioFileSourceNode](BaseAudioContext &) {
    audioFileSourceNode->pause();
  };
  audioFileSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

} // namespace audioapi
