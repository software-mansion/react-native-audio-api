#pragma once

#include "MyProcessorNode.h"
#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class MyProcessorNodeHostObject : public AudioNodeHostObject {
public:
  explicit MyProcessorNodeHostObject(
      const std::shared_ptr<MyProcessorNode> &node)
      : AudioNodeHostObject(node) {
    // addGetters(JSI_EXPORT_PROPERTY_GETTER(MyProcessorNodeHostObject, getter));
    // addSetters(JSI_EXPORT_PROPERTY_SETTER(MyProcessorNodeHostObject, setter));
    // addFunctions(JSI_EXPORT_FUNCTION(MyProcessorNodeHostObject, function));
  }

  // JSI_PROPERTY_GETTER(getter) {
  //   auto processorNode = std::static_pointer_cast<MyProcessorNode>(node_);
  //   return {processorNode->someGetter()};
  // }

  // JSI_PROPERTY_SETTER(gain) {
  //   auto processorNode = std::static_pointer_cast<MyProcessorNode>(node_);
  //   processorNode->someSetter(value.getNumber());
  // }

  // JSI_HOST_FUNCTION(function) {
  //  auto obj = args[0].getObject(runtime);
  //}
};
} // namespace audioapi
