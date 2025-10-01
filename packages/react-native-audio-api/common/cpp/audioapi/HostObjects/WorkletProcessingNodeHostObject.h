#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/effects/WorkletProcessingNode.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class WorkletProcessingNodeHostObject : public AudioNodeHostObject {
 public:
  explicit WorkletProcessingNodeHostObject(const std::shared_ptr<WorkletProcessingNode> &node)
      : AudioNodeHostObject(node) {
  }
};
} // namespace audioapi
