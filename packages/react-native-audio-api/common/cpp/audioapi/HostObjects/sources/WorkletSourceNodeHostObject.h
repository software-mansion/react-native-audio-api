#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/sources/WorkletSourceNode.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class WorkletSourceNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit WorkletSourceNodeHostObject(
      const std::shared_ptr<utils::graph::Graph> &graph,
      const std::shared_ptr<BaseAudioContext> &context,
      std::weak_ptr<worklets::WorkletRuntime> workletRuntime,
      const std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
      bool shouldLockRuntime,
      const AudioScheduledSourceNodeOptions &options = AudioScheduledSourceNodeOptions())
      : AudioScheduledSourceNodeHostObject(
            graph,
            std::make_unique<WorkletSourceNode>(
                context,
                WorkletsRunner(workletRuntime, shareableWorklet, shouldLockRuntime)),
            options) {}
};
} // namespace audioapi
