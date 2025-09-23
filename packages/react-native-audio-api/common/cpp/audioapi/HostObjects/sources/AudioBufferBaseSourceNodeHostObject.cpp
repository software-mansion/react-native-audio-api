#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>

namespace audioapi {

JSI_PROPERTY_GETTER_IMPL(AudioBufferBaseSourceNodeHostObject, detune) {
  auto sourceNode =
      std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);
  auto detune = sourceNode->getDetuneParam();
  auto detuneHostObject = std::make_shared<AudioParamHostObject>(detune);
  return jsi::Object::createFromHostObject(runtime, detuneHostObject);
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferBaseSourceNodeHostObject, playbackRate) {
  auto sourceNode =
      std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);
  auto playbackRate = sourceNode->getPlaybackRateParam();
  auto playbackRateHostObject =
      std::make_shared<AudioParamHostObject>(playbackRate);
  return jsi::Object::createFromHostObject(runtime, playbackRateHostObject);
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferBaseSourceNodeHostObject, onPositionChangedInterval) {
    auto sourceNode =
            std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);
    return jsi::Value(sourceNode->getOnPositionChangedInterval());
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferBaseSourceNodeHostObject, onPositionChanged) {
  auto sourceNode =
      std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);

  sourceNode->setOnPositionChangedCallbackId(std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferBaseSourceNodeHostObject, onPositionChangedInterval) {
  auto sourceNode =
      std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);

  sourceNode->setOnPositionChangedInterval(static_cast<int>(value.getNumber()));
}

} // namespace audioapi
