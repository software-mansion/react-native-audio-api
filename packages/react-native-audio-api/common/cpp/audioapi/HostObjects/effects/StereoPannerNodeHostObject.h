#pragma once

#include <audioapi/HostObjects/core/AudioNodeHostObject.h>
#include <audioapi/HostObjects/core/AudioParamHostObject.h>
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

  JSI_PROPERTY_GETTER_DECL(pan);
};
} // namespace audioapi
