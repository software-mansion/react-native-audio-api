#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/sources/WorkletSourceNode.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class WorkletSourceNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit WorkletSourceNodeHostObject(const std::shared_ptr<WorkletSourceNode> &node)
      : AudioScheduledSourceNodeHostObject(node) {}
};
} // namespace audioapi
