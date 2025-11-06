#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>

namespace audioapi {

AudioBufferBaseSourceNodeHostObject::AudioBufferBaseSourceNodeHostObject(
    const std::shared_ptr<AudioBufferBaseSourceNode> &node)
    : AudioScheduledSourceNodeHostObject(node) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferBaseSourceNodeHostObject, detune),
      JSI_EXPORT_PROPERTY_GETTER(
          AudioBufferBaseSourceNodeHostObject, playbackRate),
      JSI_EXPORT_PROPERTY_GETTER(
          AudioBufferBaseSourceNodeHostObject, onPositionChangedInterval));

  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(
          AudioBufferBaseSourceNodeHostObject, onPositionChanged),
      JSI_EXPORT_PROPERTY_SETTER(
          AudioBufferBaseSourceNodeHostObject, onPositionChangedInterval));
}

AudioBufferBaseSourceNodeHostObject::~AudioBufferBaseSourceNodeHostObject() {
  auto sourceNode = std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);

  // When JSI object is garbage collected (together with the eventual callback),
  // underlying source node might still be active and try to call the
  // non-existing callback.
  sourceNode->setOnPositionChangedCallbackId(0);
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferBaseSourceNodeHostObject, detune) {
  auto sourceNode = std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);
  auto detune = sourceNode->getDetuneParam();
  auto detuneHostObject = std::make_shared<AudioParamHostObject>(detune);
  return jsi::Object::createFromHostObject(runtime, detuneHostObject);
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferBaseSourceNodeHostObject, playbackRate) {
  auto sourceNode = std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);
  auto playbackRate = sourceNode->getPlaybackRateParam();
  auto playbackRateHostObject =
      std::make_shared<AudioParamHostObject>(playbackRate);
  return jsi::Object::createFromHostObject(runtime, playbackRateHostObject);
}

JSI_PROPERTY_GETTER_IMPL(
    AudioBufferBaseSourceNodeHostObject,
    onPositionChangedInterval) {
  auto sourceNode = std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);
  return {sourceNode->getOnPositionChangedInterval()};
}

JSI_PROPERTY_SETTER_IMPL(
    AudioBufferBaseSourceNodeHostObject,
    onPositionChanged) {
  auto sourceNode = std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);

  sourceNode->setOnPositionChangedCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_PROPERTY_SETTER_IMPL(
    AudioBufferBaseSourceNodeHostObject,
    onPositionChangedInterval) {
  auto sourceNode = std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);

  sourceNode->setOnPositionChangedInterval(static_cast<int>(value.getNumber()));
}

} // namespace audioapi
