#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/effects/GainNode.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class GainNodeHostObject : public AudioNodeHostObject {
 public:
  explicit GainNodeHostObject(const std::shared_ptr<GainNode> &node)
      : AudioNodeHostObject(node) {
    addGetters(JSI_EXPORT_PROPERTY_GETTER(GainNodeHostObject, gain));
  }

  JSI_PROPERTY_GETTER(gain) {
    auto gainNode = std::static_pointer_cast<GainNode>(node_);
    auto gainParam =
        std::make_shared<AudioParamHostObject>(gainNode->getGainParam());
    return jsi::Object::createFromHostObject(runtime, gainParam);
  }
};
} // namespace audioapi
