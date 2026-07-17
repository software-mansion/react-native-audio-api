#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/destinations/AudioDestinationNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/core/AudioNode.h>

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

std::shared_ptr<utils::graph::HostNode> AudioNodeHostObject::getConnectSource(int /*outputIndex*/) {
  return shared_from_this();
}

std::shared_ptr<utils::graph::HostNode> AudioNodeHostObject::getConnectDestination(
    int /*inputIndex*/) {
  return shared_from_this();
}

JSI_HOST_FUNCTION_IMPL(AudioNodeHostObject, connect) {
  auto obj = args[0].getObject(runtime);

  const int output = (count > 1 && args[1].isNumber()) ? static_cast<int>(args[1].getNumber()) : 0;
  const int input = (count > 2 && args[2].isNumber()) ? static_cast<int>(args[2].getNumber()) : 0;

  if (output < 0 || output >= numberOfOutputs_) {
    throw jsi::JSError(runtime, "IndexSizeError: connect() output index out of bounds");
  }

  if (obj.isHostObject<AudioNodeHostObject>(runtime)) {
    auto toNodeHost = obj.getHostObject<AudioNodeHostObject>(runtime);

    if (input < 0 || input >= toNodeHost->numberOfInputs_) {
      throw jsi::JSError(runtime, "IndexSizeError: connect() input index out of bounds");
    }

    auto source = getConnectSource(output);
    auto destination = toNodeHost->getConnectDestination(input);
    // Duplicate connections are a no-op per the Web Audio spec, so any Err
    // (EDGE_ALREADY_EXISTS, cycle, ...) from the graph is intentionally ignored.
    (void)source->connect(*destination);
  } else if (obj.isHostObject<AudioParamHostObject>(runtime)) {
    auto param = obj.getHostObject<AudioParamHostObject>(runtime);
    param->connectToGraph();
    graph_->addEdge(getConnectSource(output)->rawNode(), param->bridgeNode());
  }
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioNodeHostObject, disconnect) {
  // disconnect() — drop every outgoing edge from every output.
  if (args == nullptr || count == 0 || args[0].isUndefined()) {
    for (int output = 0; output < numberOfOutputs_; ++output) {
      (void)getConnectSource(output)->disconnect();
    }
    return jsi::Value::undefined();
  }

  // disconnect(output) — drop every outgoing edge from a single output.
  if (args[0].isNumber()) {
    const int output = static_cast<int>(args[0].getNumber());
    if (output < 0 || output >= numberOfOutputs_) {
      throw jsi::JSError(runtime, "IndexSizeError: disconnect() output index out of bounds");
    }
    (void)getConnectSource(output)->disconnect();
    return jsi::Value::undefined();
  }

  auto obj = args[0].getObject(runtime);
  const bool hasOutput = count > 1 && args[1].isNumber();
  const bool hasInput = count > 2 && args[2].isNumber();
  const int output = hasOutput ? static_cast<int>(args[1].getNumber()) : 0;
  const int input = hasInput ? static_cast<int>(args[2].getNumber()) : 0;

  if (hasOutput && (output < 0 || output >= numberOfOutputs_)) {
    throw jsi::JSError(runtime, "IndexSizeError: disconnect() output index out of bounds");
  }

  const int outputBegin = hasOutput ? output : 0;
  const int outputEnd = hasOutput ? output + 1 : numberOfOutputs_;

  if (obj.isHostObject<AudioNodeHostObject>(runtime)) {
    auto toNodeHost = obj.getHostObject<AudioNodeHostObject>(runtime);

    if (hasInput && (input < 0 || input >= toNodeHost->numberOfInputs_)) {
      throw jsi::JSError(runtime, "IndexSizeError: disconnect() input index out of bounds");
    }

    const int inputBegin = hasInput ? input : 0;
    const int inputEnd = hasInput ? input + 1 : toNodeHost->numberOfInputs_;

    for (int o = outputBegin; o < outputEnd; ++o) {
      auto source = getConnectSource(o);
      for (int i = inputBegin; i < inputEnd; ++i) {
        (void)source->disconnect(*toNodeHost->getConnectDestination(i));
      }
    }
  } else if (obj.isHostObject<AudioParamHostObject>(runtime)) {
    auto param = obj.getHostObject<AudioParamHostObject>(runtime);
    for (int o = outputBegin; o < outputEnd; ++o) {
      graph_->removeEdge(getConnectSource(o)->rawNode(), param->bridgeNode());
    }
  }

  return jsi::Value::undefined();
}
} // namespace audioapi
