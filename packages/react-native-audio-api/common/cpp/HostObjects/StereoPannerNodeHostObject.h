#pragma once

#include <memory>
#include <string>
#include <vector>

#include "AudioNodeHostObject.h"
#include "AudioParamHostObject.h"
#include "StereoPannerNode.h"

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
