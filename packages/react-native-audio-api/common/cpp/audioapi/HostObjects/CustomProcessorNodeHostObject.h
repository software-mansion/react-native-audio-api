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
        JSI_EXPORT_PROPERTY_GETTER(CustomProcessorNodeHostObject, processorMode));

    addSetters(
        JSI_EXPORT_PROPERTY_SETTER(CustomProcessorNodeHostObject, processorMode));
  }

  JSI_PROPERTY_GETTER(customProcessor) {
    auto customProcessorNode = std::static_pointer_cast<CustomProcessorNode>(node_);
    auto customProcessorParam =
        std::make_shared<AudioParamHostObject>(customProcessorNode->getCustomProcessorParam());
    return jsi::Object::createFromHostObject(runtime, customProcessorParam);
  }

  JSI_PROPERTY_GETTER(processorMode) {
    auto customProcessorNode = std::static_pointer_cast<CustomProcessorNode>(node_);
    auto mode = customProcessorNode->getProcessorMode();
    std::string modeStr = (mode == CustomProcessorNode::ProcessorMode::ProcessThrough)
                            ? "processThrough"
                            : "processInPlace";
    return jsi::String::createFromUtf8(runtime, modeStr);
  }

  JSI_PROPERTY_SETTER(processorMode) {
    auto customProcessorNode = std::static_pointer_cast<CustomProcessorNode>(node_);
    std::string modeStr = value.getString(runtime).utf8(runtime);

    if (modeStr == "processThrough") {
      customProcessorNode->setProcessorMode(CustomProcessorNode::ProcessorMode::ProcessThrough);
    } else {
      customProcessorNode->setProcessorMode(CustomProcessorNode::ProcessorMode::ProcessInPlace);
    }
  }
};

} // namespace audioapi
