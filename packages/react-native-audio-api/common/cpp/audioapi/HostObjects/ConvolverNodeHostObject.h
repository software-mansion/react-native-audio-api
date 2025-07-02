#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/HostObjects/AudioBufferHostObject.h>
#include <iostream>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class ConvolverNodeHostObject : public AudioNodeHostObject {
 public:
  explicit ConvolverNodeHostObject(const std::shared_ptr<ConvolverNode> &node)
      : AudioNodeHostObject(node) {
    addGetters(JSI_EXPORT_PROPERTY_GETTER(ConvolverNodeHostObject, normalize),
               JSI_EXPORT_PROPERTY_GETTER(ConvolverNodeHostObject, buffer));
    addSetters(
        JSI_EXPORT_PROPERTY_SETTER(ConvolverNodeHostObject, normalize),
        JSI_EXPORT_PROPERTY_SETTER(ConvolverNodeHostObject, buffer));
  }

  JSI_PROPERTY_GETTER(normalize) {
    auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
    return {convolverNode->getNormalize_()};
  }

  JSI_PROPERTY_GETTER(buffer) {
    auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
    auto buffer = convolverNode->getBuffer();
    auto bufferHostObject =
        std::make_shared<AudioBufferHostObject>(buffer);
    return jsi::Object::createFromHostObject(runtime, bufferHostObject);
  }

  JSI_PROPERTY_SETTER(normalize) {
    auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
    convolverNode->setNormalize(value.getBool());
  }

  JSI_PROPERTY_SETTER(buffer) {
    auto convolverNode =
        std::static_pointer_cast<ConvolverNode>(node_);
    if (value.isNull()) {
      convolverNode->setBuffer(std::shared_ptr<AudioBuffer>(nullptr));
      return;
    }

    auto bufferHostObject =
        value.getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
    convolverNode->setBuffer(bufferHostObject->audioBuffer_);
  }
};
} // namespace audioapi
