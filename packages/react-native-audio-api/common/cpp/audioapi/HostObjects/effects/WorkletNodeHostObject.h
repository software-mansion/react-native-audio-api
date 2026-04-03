#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/effects/WorkletNode.h>
#include <audioapi/core/utils/graph/Graph.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class WorkletNodeHostObject : public AudioNodeHostObject {
 public:
  explicit WorkletNodeHostObject(
      const std::shared_ptr<utils::graph::Graph> &graph,
      const std::shared_ptr<BaseAudioContext> &context,
      std::weak_ptr<worklets::WorkletRuntime> workletRuntime,
      const std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
      bool shouldLockRuntime,
      size_t bufferLength,
      size_t inputChannelCount)
      : AudioNodeHostObject(
            graph,
            std::make_unique<WorkletNode>(
                context,
                bufferLength,
                inputChannelCount,
                WorkletsRunner(workletRuntime, shareableWorklet, shouldLockRuntime))) {}
};
} // namespace audioapi
