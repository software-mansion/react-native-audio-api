#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/core/WorkletNode.h>
#include <worklets/Compat/StableApi.h>

#include <memory>
#include <utility>

namespace audioworklets {

using namespace facebook;

class WorkletNodeHostObject : public audioapi::AudioNodeHostObject {
 public:
  WorkletNodeHostObject(
      const std::shared_ptr<audioapi::utils::graph::Graph> &graph,
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      std::shared_ptr<worklets::WorkletRuntime> uiRuntime,
      std::shared_ptr<worklets::UIScheduler> uiScheduler,
      std::shared_ptr<worklets::Serializable> serializableWorklet)
      : audioapi::AudioNodeHostObject(
            graph,
            std::make_unique<WorkletNode>(
                context,
                UIWorkletsRunner(
                    std::move(uiRuntime),
                    std::move(uiScheduler),
                    std::move(serializableWorklet)))) {}

  [[nodiscard]] size_t getMemoryPressure() const override {
    return AudioNodeHostObject::getMemoryPressure() +
        static_cast<size_t>(audioapi::MAX_CHANNEL_COUNT) * audioapi::RENDER_QUANTUM_SIZE *
        sizeof(float);
  }
};

} // namespace audioworklets
