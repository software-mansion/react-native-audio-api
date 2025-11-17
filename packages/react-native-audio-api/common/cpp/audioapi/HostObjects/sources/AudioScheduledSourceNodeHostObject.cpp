#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>

#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <memory>

namespace audioapi {

AudioScheduledSourceNodeHostObject::AudioScheduledSourceNodeHostObject(
    const std::shared_ptr<AudioScheduledSourceNode> &node)
    : AudioNodeHostObject(node) {
  addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioScheduledSourceNodeHostObject, onEnded));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, start),
      JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, stop));
}

AudioScheduledSourceNodeHostObject::~AudioScheduledSourceNodeHostObject() {
  auto audioScheduledSourceNode = std::static_pointer_cast<AudioScheduledSourceNode>(node_);

  // When JSI object is garbage collected (together with the eventual callback),
  // underlying source node might still be active and try to call the
  // non-existing callback.
  audioScheduledSourceNode->setOnEndedCallbackId(0);
}

JSI_PROPERTY_SETTER_IMPL(AudioScheduledSourceNodeHostObject, onEnded) {
  auto audioScheduleSourceNode = std::static_pointer_cast<AudioScheduledSourceNode>(node_);

  audioScheduleSourceNode->setOnEndedCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_HOST_FUNCTION_IMPL(AudioScheduledSourceNodeHostObject, start) {
  auto when = args[0].getNumber();
  auto audioScheduleSourceNode = std::static_pointer_cast<AudioScheduledSourceNode>(node_);
  audioScheduleSourceNode->start(when);
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioScheduledSourceNodeHostObject, stop) {
  auto time = args[0].getNumber();
  auto audioScheduleSourceNode = std::static_pointer_cast<AudioScheduledSourceNode>(node_);
  audioScheduleSourceNode->stop(time);
  return jsi::Value::undefined();
}

} // namespace audioapi
