#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/effects/CustomProcessorNode.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class CustomProcessorNodeHostObject : public AudioNodeHostObject {
 public:
  explicit CustomProcessorNodeHostObject(const std::shared_ptr<CustomProcessorNode> &node)
      : AudioNodeHostObject(node) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(CustomProcessorNodeHostObject, customProcessor),
        JSI_EXPORT_PROPERTY_GETTER(CustomProcessorNodeHostObject, identifier),
        JSI_EXPORT_PROPERTY_GETTER(CustomProcessorNodeHostObject, processorMode));

    addSetters(
        JSI_EXPORT_PROPERTY_SETTER(CustomProcessorNodeHostObject, identifier),
        JSI_EXPORT_PROPERTY_SETTER(CustomProcessorNodeHostObject, processorMode));
  }

  JSI_PROPERTY_GETTER(customProcessor) {
    auto customProcessorNode = std::static_pointer_cast<CustomProcessorNode>(node_);
    auto customProcessorParam =
        std::make_shared<AudioParamHostObject>(customProcessorNode->getCustomProcessorParam());
    return jsi::Object::createFromHostObject(runtime, customProcessorParam);
  }

  JSI_PROPERTY_GETTER(identifier) {
    auto customProcessorNode = std::static_pointer_cast<CustomProcessorNode>(node_);
    return jsi::String::createFromUtf8(runtime, customProcessorNode->getIdentifier());
  }

  JSI_PROPERTY_SETTER(identifier) {
    auto customProcessorNode = std::static_pointer_cast<CustomProcessorNode>(node_);
    customProcessorNode->setIdentifier(value.getString(runtime).utf8(runtime));
  }

  JSI_PROPERTY_GETTER(processorMode) {
    auto customProcessorNode = std::static_pointer_cast<CustomProcessorNode>(node_);
    return jsi::String::createFromUtf8(runtime, customProcessorNode->getProcessorMode());
  }

  JSI_PROPERTY_SETTER(processorMode) {
    auto customProcessorNode = std::static_pointer_cast<CustomProcessorNode>(node_);
    customProcessorNode->setProcessorMode(value.getString(runtime).utf8(runtime));
  }
};

} // namespace audioapi