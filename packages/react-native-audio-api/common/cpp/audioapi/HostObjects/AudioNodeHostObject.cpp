#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
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
