#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/core/WorkletNode.h>
#include <audioworklets/core/WorkletNodeDomain.h>
#include <worklets/Compat/StableApi.h>

#include <memory>

namespace audioworklets {

using namespace facebook;

class WorkletNodeHostObject : public audioapi::AudioNodeHostObject {
 public:
  WorkletNodeHostObject(
      const std::shared_ptr<audioapi::utils::graph::Graph> &graph,
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      const std::shared_ptr<worklets::WorkletRuntime> &uiRuntime,
      const std::shared_ptr<worklets::UIScheduler> &uiScheduler,
      const std::shared_ptr<worklets::Serializable> &serializableWorklet,
      WorkletNodeDomain domain,
      const WorkletNodeOptions &options);

  JSI_PROPERTY_GETTER_DECL(bufferLength);
  JSI_PROPERTY_GETTER_DECL(smoothingTimeConstant);

  JSI_PROPERTY_SETTER_DECL(smoothingTimeConstant);

  [[nodiscard]] size_t getMemoryPressure() const override;

 private:
  WorkletNode *workletNode_ = nullptr;
};

} // namespace audioworklets
