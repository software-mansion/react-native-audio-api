#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <utility>

namespace audioapi {

AudioScheduledSourceNodeHostObject::AudioScheduledSourceNodeHostObject(
    const std::shared_ptr<utils::graph::Graph> &graph,
    std::unique_ptr<AudioNode> node,
    const AudioScheduledSourceNodeOptions &options)
    : AudioNodeHostObject(graph, std::move(node), options),
      scheduledSourceNode_(typedAudioNode<AudioScheduledSourceNode>(node_)) {
  addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioScheduledSourceNodeHostObject, onEnded));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, start),
      JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, stop));
}

AudioScheduledSourceNodeHostObject::~AudioScheduledSourceNodeHostObject() {
  // When JSI object is garbage collected (together with the eventual callback),
  // underlying source node might still be active and try to call the
  // non-existing callback.
  scheduledSourceNode_->assignOnEndedCallbackId(0);
}

JSI_PROPERTY_SETTER_IMPL(AudioScheduledSourceNodeHostObject, onEnded) {
  scheduledSourceNode_->assignOnEndedCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_HOST_FUNCTION_IMPL(AudioScheduledSourceNodeHostObject, start) {
  auto handle = node_->handle;
  auto event =
      [handle, node = scheduledSourceNode_, when = args[0].getNumber()](BaseAudioContext &) {
        node->start(when);
      };
  scheduledSourceNode_->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioScheduledSourceNodeHostObject, stop) {
  auto handle = node_->handle;
  auto event =
      [handle, node = scheduledSourceNode_, when = args[0].getNumber()](BaseAudioContext &) {
        node->stop(when);
      };
  scheduledSourceNode_->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

} // namespace audioapi
