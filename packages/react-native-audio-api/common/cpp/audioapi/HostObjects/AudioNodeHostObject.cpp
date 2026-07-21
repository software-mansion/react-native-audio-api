#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/HostObjects/destinations/AudioDestinationNodeHostObject.h>
#include <audioapi/HostObjects/effects/DelayNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/effects/delay/host_nodes/DelayReaderHostNode.h>
#include <audioapi/core/effects/delay/host_nodes/DelayWriterHostNode.h>

#include <memory>
#include <utility>

namespace audioapi {

AudioNodeHostObject::AudioNodeHostObject(
    const std::shared_ptr<utils::graph::Graph> &graph,
    std::unique_ptr<AudioNode> node,
    const AudioNodeOptions &options)
    : utils::graph::HostNode(graph, std::move(node)),
      audioNode_(typedAudioNode<AudioNode>(node_)),
      numberOfInputs_(options.numberOfInputs),
      numberOfOutputs_(options.numberOfOutputs),
      channelCount_(options.channelCount),
      channelCountMode_(options.channelCountMode),
      channelInterpretation_(options.channelInterpretation) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, numberOfInputs),
      JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, numberOfOutputs),
      JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, channelCount),
      JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, channelCountMode),
      JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, channelInterpretation));

  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AudioNodeHostObject, channelCount),
      JSI_EXPORT_PROPERTY_SETTER(AudioNodeHostObject, channelCountMode),
      JSI_EXPORT_PROPERTY_SETTER(AudioNodeHostObject, channelInterpretation));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioNodeHostObject, connect),
      JSI_EXPORT_FUNCTION(AudioNodeHostObject, disconnect));
}

// Explicitly define destructor here, as they to exist in order to act as a
// "key function" for the audio classes - this allow for RTTI to work
// properly across dynamic library boundaries (i.e. dynamic_cast that is used by
// isHostObject method), android specific issue
AudioNodeHostObject::~AudioNodeHostObject() = default;

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, numberOfInputs) {
  return {numberOfInputs_};
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, numberOfOutputs) {
  return {numberOfOutputs_};
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, channelCount) {
  return {static_cast<int>(channelCount_)};
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, channelCountMode) {
  return jsi::String::createFromUtf8(
      runtime, js_enum_parser::channelCountModeToString(channelCountMode_));
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, channelInterpretation) {
  return jsi::String::createFromUtf8(
      runtime, js_enum_parser::channelInterpretationToString(channelInterpretation_));
}

JSI_PROPERTY_SETTER_IMPL(AudioNodeHostObject, channelCount) {
  const auto count = value.getNumber();
  // Per spec channelCount must be a positive integer; the JS layer throws the
  // DOM error. Guard defensively here — the `!(count >= 1)` form also rejects
  // NaN (NaN compares false to everything).
  if (!(count >= 1)) {
    return;
  }

  updateChannelCount(static_cast<size_t>(count));
}

void AudioNodeHostObject::updateChannelCount(size_t newChannelCount) {
  if (newChannelCount == channelCount_) {
    return;
  }
  // Negotiation reads channelCount on the host thread, so set it here (not on
  // the audio thread) before recomputing layouts.
  audioNode_->setChannelCount(newChannelCount);
  channelCount_ = newChannelCount;
  renegotiate();
}

JSI_PROPERTY_SETTER_IMPL(AudioNodeHostObject, channelCountMode) {
  ChannelCountMode parsedMode;
  try {
    parsedMode = js_enum_parser::channelCountModeFromString(value.toString(runtime).utf8(runtime));
  } catch (const std::invalid_argument &) {
    // WebIDL coerces non-strings; unknown enum values are ignored.
    return;
  }

  if (parsedMode == channelCountMode_) {
    return;
  }

  // channelCountMode is read only on the host thread during negotiation.
  audioNode_->setChannelCountMode(parsedMode);
  channelCountMode_ = parsedMode;
  renegotiate();
}

JSI_PROPERTY_SETTER_IMPL(AudioNodeHostObject, channelInterpretation) {
  ChannelInterpretation parsedInterpretation;
  try {
    parsedInterpretation =
        js_enum_parser::channelInterpretationFromString(value.toString(runtime).utf8(runtime));
  } catch (const std::invalid_argument &) {
    // WebIDL coerces non-strings; unknown enum values are ignored.
    return;
  }

  // channelInterpretation is read on the audio thread (up/down-mix summing in
  // processInputs), so apply it there via a scheduled event. It does not change
  // the channel count, so no renegotiation is needed. Capture the NodeHandle to
  // keep the payload alive until the deferred event runs.
  audioNode_->scheduleAudioEvent(
      [handle = node_->handle, node = audioNode_, parsedInterpretation](BaseAudioContext &) {
        node->setChannelInterpretation(parsedInterpretation);
      });
  channelInterpretation_ = parsedInterpretation;
}

JSI_HOST_FUNCTION_IMPL(AudioNodeHostObject, connect) {
  auto obj = args[0].getObject(runtime);

  if (obj.isHostObject<AudioNodeHostObject>(runtime)) {
    auto toNodeHost = obj.getHostObject<AudioNodeHostObject>(runtime);

    // source is a delay node: route through its reader
    if (auto *fromDelay = dynamic_cast<DelayNodeHostObject *>(this); fromDelay != nullptr) {
      fromDelay->delayReaderHostNode_->connect(*toNodeHost);
      return jsi::Value::undefined();
    }

    // destination is a delay node: route through its writer
    if (auto *toDelay = dynamic_cast<DelayNodeHostObject *>(toNodeHost.get()); toDelay != nullptr) {
      connect(*toDelay->delayWriterHostNode_);
      return jsi::Value::undefined();
    }

    connect(*toNodeHost);
  } else if (obj.isHostObject<AudioParamHostObject>(runtime)) {
    auto param = obj.getHostObject<AudioParamHostObject>(runtime);
    param->connectToGraph();
    graph_->addEdge(node_, param->bridgeNode());
  }
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioNodeHostObject, disconnect) {
  // protect direct usage of raw jsi classes
  if (args == nullptr || args[0].isUndefined()) {
    disconnect();
    return jsi::Value::undefined();
  }

  auto obj = args[0].getObject(runtime);
  if (obj.isHostObject<AudioNodeHostObject>(runtime)) {
    auto node = obj.getHostObject<AudioNodeHostObject>(runtime);
    disconnect(*node);
  } else if (obj.isHostObject<AudioParamHostObject>(runtime)) {
    auto param = obj.getHostObject<AudioParamHostObject>(runtime);
    // Disconnect source → bridge
    graph_->removeEdge(node_, param->bridgeNode());
  }

  return jsi::Value::undefined();
}
} // namespace audioapi
