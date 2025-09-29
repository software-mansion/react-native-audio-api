#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/AudioNode.h>

namespace audioapi {

AudioNodeHostObject::AudioNodeHostObject(const std::shared_ptr<AudioNode> &node)
    : node_(node) {
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

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, numberOfInputs) {
  return {node_->getNumberOfInputs()};
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, numberOfOutputs) {
  return {node_->getNumberOfOutputs()};
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, channelCount) {
  return {node_->getChannelCount()};
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, channelCountMode) {
  return jsi::String::createFromUtf8(runtime, node_->getChannelCountMode());
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, channelInterpretation) {
  return jsi::String::createFromUtf8(
      runtime, node_->getChannelInterpretation());
}

JSI_HOST_FUNCTION_IMPL(AudioNodeHostObject, connect) {
  auto obj = args[0].getObject(runtime);
  if (obj.isHostObject<AudioNodeHostObject>(runtime)) {
    auto node = obj.getHostObject<AudioNodeHostObject>(runtime);
    node_->connect(std::shared_ptr<AudioNodeHostObject>(node)->node_);
  }
  if (obj.isHostObject<AudioParamHostObject>(runtime)) {
    auto param = obj.getHostObject<AudioParamHostObject>(runtime);
    node_->connect(std::shared_ptr<AudioParamHostObject>(param)->param_);
  }
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioNodeHostObject, disconnect) {
  if (args[0].isUndefined()) {
    node_->disconnect();
    return jsi::Value::undefined();
  }
  auto obj = args[0].getObject(runtime);
  if (obj.isHostObject<AudioNodeHostObject>(runtime)) {
    auto node = obj.getHostObject<AudioNodeHostObject>(runtime);
    node_->disconnect(std::shared_ptr<AudioNodeHostObject>(node)->node_);
  }

  if (obj.isHostObject<AudioParamHostObject>(runtime)) {
    auto param = obj.getHostObject<AudioParamHostObject>(runtime);
    node_->disconnect(std::shared_ptr<AudioParamHostObject>(param)->param_);
  }
  return jsi::Value::undefined();
}
} // namespace audioapi
