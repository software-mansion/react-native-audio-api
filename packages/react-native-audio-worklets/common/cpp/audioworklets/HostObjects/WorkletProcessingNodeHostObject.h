#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/AudioWorkletsRunner.h>
#include <audioworklets/core/WorkletProcessingNode.h>

#include <memory>
#include <utility>

namespace audioworklets {

using namespace facebook;

class WorkletProcessingNodeHostObject : public audioapi::AudioNodeHostObject {
 public:
  WorkletProcessingNodeHostObject(
      const std::shared_ptr<audioapi::utils::graph::Graph> &graph,
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      std::weak_ptr<worklets::WorkletRuntime> workletRuntime,
      const std::shared_ptr<worklets::Serializable> &serializableWorklet)
      : audioapi::AudioNodeHostObject(
            graph,
            std::make_unique<WorkletProcessingNode>(
                context,
                AudioWorkletsRunner(std::move(workletRuntime), serializableWorklet))) {}

  [[nodiscard]] size_t getMemoryPressure() const override {
    return AudioNodeHostObject::getMemoryPressure() +
        2 * static_cast<size_t>(audioapi::MAX_CHANNEL_COUNT) * audioapi::RENDER_QUANTUM_SIZE *
        sizeof(float);
  }
};

} // namespace audioworklets
