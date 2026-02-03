#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/effects/WorkletNode.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class WorkletNodeHostObject : public AudioNodeHostObject {
 public:
  explicit WorkletNodeHostObject(const std::shared_ptr<WorkletNode> &node)
      : AudioNodeHostObject(node) {}
};
} // namespace audioapi
