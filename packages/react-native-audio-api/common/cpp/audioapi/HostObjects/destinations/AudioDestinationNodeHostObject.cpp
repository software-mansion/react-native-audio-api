#include <audioapi/HostObjects/destinations/AudioDestinationNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>

#include <memory>
#include <utility>

namespace audioapi {

AudioDestinationNodeHostObject::AudioDestinationNodeHostObject(
    utils::graph::HostGraph::Node *node,
    std::shared_ptr<AudioDestinationNode> destination,
    const AudioDestinationOptions &options)
    : node_(node),
      destination_(std::move(destination)),
      numberOfInputs_(options.numberOfInputs),
      numberOfOutputs_(options.numberOfOutputs),
      channelCount_(options.channelCount),
      channelCountMode_(options.channelCountMode),
      channelInterpretation_(options.channelInterpretation) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioDestinationNodeHostObject, numberOfInputs),
      JSI_EXPORT_PROPERTY_GETTER(AudioDestinationNodeHostObject, numberOfOutputs),
      JSI_EXPORT_PROPERTY_GETTER(AudioDestinationNodeHostObject, channelCount),
      JSI_EXPORT_PROPERTY_GETTER(AudioDestinationNodeHostObject, channelCountMode),
      JSI_EXPORT_PROPERTY_GETTER(AudioDestinationNodeHostObject, channelInterpretation));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioDestinationNodeHostObject, connect),
      JSI_EXPORT_FUNCTION(AudioDestinationNodeHostObject, disconnect));
}

JSI_PROPERTY_GETTER_IMPL(AudioDestinationNodeHostObject, numberOfInputs) {
  return {numberOfInputs_};
}

JSI_PROPERTY_GETTER_IMPL(AudioDestinationNodeHostObject, numberOfOutputs) {
  return {numberOfOutputs_};
}

JSI_PROPERTY_GETTER_IMPL(AudioDestinationNodeHostObject, channelCount) {
  return {static_cast<int>(channelCount_)};
}

JSI_PROPERTY_GETTER_IMPL(AudioDestinationNodeHostObject, channelCountMode) {
  return jsi::String::createFromUtf8(
      runtime, js_enum_parser::channelCountModeToString(channelCountMode_));
}

JSI_PROPERTY_GETTER_IMPL(AudioDestinationNodeHostObject, channelInterpretation) {
  return jsi::String::createFromUtf8(
      runtime, js_enum_parser::channelInterpretationToString(channelInterpretation_));
}

/// AudioDestinationNode is the end point of the audio graph, it cannot connect to any other node, so connect and disconnect are no-op
JSI_HOST_FUNCTION_IMPL(AudioDestinationNodeHostObject, connect) {
  return jsi::Value::undefined();
}

/// AudioDestinationNode is the end point of the audio graph, it cannot connect to any other node, so connect and disconnect are no-op
JSI_HOST_FUNCTION_IMPL(AudioDestinationNodeHostObject, disconnect) {
  return jsi::Value::undefined();
}

} // namespace audioapi
