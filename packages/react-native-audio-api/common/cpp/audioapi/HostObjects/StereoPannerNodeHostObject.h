#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/effects/StereoPannerNode.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class StereoPannerNodeHostObject : public AudioNodeHostObject {
 public:
  explicit StereoPannerNodeHostObject(
      const std::shared_ptr<StereoPannerNode> &node)
      : AudioNodeHostObject(node) {
    addGetters(JSI_EXPORT_PROPERTY_GETTER(StereoPannerNodeHostObject, pan));
  }

  JSI_PROPERTY_GETTER(pan) {
    auto stereoPannerNode = std::static_pointer_cast<StereoPannerNode>(node_);
    auto panParam_ =
        std::make_shared<AudioParamHostObject>(stereoPannerNode->getPanParam());
    return jsi::Object::createFromHostObject(runtime, panParam_);
  }
};
} // namespace audioapi
