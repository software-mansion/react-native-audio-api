#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/effects/WorkletProcessingNode.h>
#include <audioapi/core/utils/graph/Graph.h>

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

  [[nodiscard]] size_t getMemoryPressure() const override {
    // 2 input + 2 output AudioArrayBuffer (RQ floats) wrapped as JS typed arrays.
    // The worklet closure/captures live in JS and aren't tracked here.
    return AudioNodeHostObject::getMemoryPressure() + 4 * RENDER_QUANTUM_SIZE * sizeof(float);
  }
};
} // namespace audioapi
