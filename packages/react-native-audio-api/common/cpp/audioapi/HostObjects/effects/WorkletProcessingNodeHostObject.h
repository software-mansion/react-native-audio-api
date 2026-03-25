#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/effects/WorkletProcessingNode.h>
#include <audioapi/core/utils/graph/Graph.hpp>

#include <memory>

namespace audioapi {
using namespace facebook;

class WorkletProcessingNodeHostObject : public AudioNodeHostObject {
 public:
  explicit WorkletProcessingNodeHostObject(
      const std::shared_ptr<utils::graph::Graph> &graph,
      const std::shared_ptr<BaseAudioContext> &context,
      std::weak_ptr<worklets::WorkletRuntime> workletRuntime,
      const std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
      bool shouldLockRuntime)
      : AudioNodeHostObject(
            graph,
            std::make_unique<WorkletProcessingNode>(
                context,
                WorkletsRunner(workletRuntime, shareableWorklet, shouldLockRuntime))) {}
};
} // namespace audioapi
