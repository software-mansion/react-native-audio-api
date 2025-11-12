#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class DelayNode;

class DelayNodeHostObject : public AudioNodeHostObject {
 public:
  explicit DelayNodeHostObject(const std::shared_ptr<DelayNode> &node);

  JSI_PROPERTY_GETTER_DECL(delayTime);
};
} // namespace audioapi
