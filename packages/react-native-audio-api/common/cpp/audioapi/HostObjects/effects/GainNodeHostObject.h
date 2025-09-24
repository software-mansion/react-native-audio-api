#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class GainNode;

class GainNodeHostObject : public AudioNodeHostObject {
 public:
  explicit GainNodeHostObject(const std::shared_ptr<GainNode> &node);

  JSI_PROPERTY_GETTER_DECL(gain);
};
} // namespace audioapi
