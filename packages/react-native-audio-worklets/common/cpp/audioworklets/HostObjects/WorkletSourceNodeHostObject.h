#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/AudioWorkletsRunner.h>
#include <audioworklets/core/WorkletSourceNode.h>

#include <memory>
#include <utility>

namespace audioworklets {

using namespace facebook;

class WorkletSourceNodeHostObject : public audioapi::AudioScheduledSourceNodeHostObject {
 public:
  WorkletSourceNodeHostObject(
      const std::shared_ptr<audioapi::utils::graph::Graph> &graph,
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      std::weak_ptr<worklets::WorkletRuntime> workletRuntime,
      const std::shared_ptr<worklets::Serializable> &serializableWorklet)
      : audioapi::AudioScheduledSourceNodeHostObject(
            graph,
            std::make_unique<WorkletSourceNode>(
                context,
                AudioWorkletsRunner(std::move(workletRuntime), serializableWorklet))) {}

  [[nodiscard]] size_t getMemoryPressure() const override {
    return AudioNodeHostObject::getMemoryPressure() +
        static_cast<size_t>(audioapi::MAX_CHANNEL_COUNT) * audioapi::RENDER_QUANTUM_SIZE *
        sizeof(float);
  }
};

} // namespace audioworklets
