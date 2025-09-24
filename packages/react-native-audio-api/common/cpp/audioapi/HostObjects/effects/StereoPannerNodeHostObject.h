#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class StereoPannerNode;

class StereoPannerNodeHostObject : public AudioNodeHostObject {
 public:
  explicit StereoPannerNodeHostObject(
      const std::shared_ptr<StereoPannerNode> &node);

  JSI_PROPERTY_GETTER_DECL(pan);
};
} // namespace audioapi
