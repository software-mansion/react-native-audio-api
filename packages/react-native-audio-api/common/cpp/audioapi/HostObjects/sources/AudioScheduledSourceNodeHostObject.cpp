#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <utility>

namespace audioapi {

AudioScheduledSourceNodeHostObject::AudioScheduledSourceNodeHostObject(
    const std::shared_ptr<AudioScheduledSourceNode> &node,
    const AudioScheduledSourceNodeOptions &options)
    : AudioNodeHostObject(node) {
  addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioScheduledSourceNodeHostObject, onEnded));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, start),
      JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, stop));
}

AudioScheduledSourceNodeHostObject::~AudioScheduledSourceNodeHostObject() {
  // When JSI object is garbage collected (together with the eventual callback),
  // underlying source node might still be active and try to call the
  // non-existing callback.
  setOnEndedCallbackId(0);
}

JSI_PROPERTY_SETTER_IMPL(AudioScheduledSourceNodeHostObject, onEnded) {
  auto callbackId = std::stoull(value.getString(runtime).utf8(runtime));
  setOnEndedCallbackId(callbackId);
}

JSI_HOST_FUNCTION_IMPL(AudioScheduledSourceNodeHostObject, start) {
  auto audioScheduleSourceNode = std::static_pointer_cast<AudioScheduledSourceNode>(node_);

  auto event = [audioScheduleSourceNode, when = args[0].getNumber()](BaseAudioContext &) {
    audioScheduleSourceNode->start(when);
  };
  audioScheduleSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioScheduledSourceNodeHostObject, stop) {
  auto audioScheduleSourceNode = std::static_pointer_cast<AudioScheduledSourceNode>(node_);

  auto event = [audioScheduleSourceNode, when = args[0].getNumber()](BaseAudioContext &) {
    audioScheduleSourceNode->stop(when);
  };
  audioScheduleSourceNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

void AudioScheduledSourceNodeHostObject::setOnEndedCallbackId(uint64_t callbackId) {
  auto sourceNode = std::static_pointer_cast<AudioScheduledSourceNode>(node_);

  auto event = [sourceNode, callbackId](BaseAudioContext &) {
    sourceNode->setOnEndedCallbackId(callbackId);
  };

  sourceNode->unregisterOnEndedCallback(onEndedCallbackId_);
  sourceNode->scheduleAudioEvent(std::move(event));
  onEndedCallbackId_ = callbackId;
}

} // namespace audioapi
