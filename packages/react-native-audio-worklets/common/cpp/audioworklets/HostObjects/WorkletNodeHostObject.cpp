#include <audioworklets/HostObjects/WorkletNodeHostObject.h>

#include <audioapi/HostObjects/TypedAudioNodePtr.hpp>
#include <audioapi/core/BaseAudioContext.h>

#include <memory>

namespace audioworklets {

WorkletNodeHostObject::WorkletNodeHostObject(
    const std::shared_ptr<audioapi::utils::graph::Graph> &graph,
    const std::shared_ptr<audioapi::BaseAudioContext> &context,
    const std::shared_ptr<worklets::WorkletRuntime> &uiRuntime,
    const std::shared_ptr<worklets::UIScheduler> &uiScheduler,
    const std::shared_ptr<worklets::Serializable> &serializableWorklet,
    WorkletNodeDomain domain,
    const WorkletNodeOptions &options)
    : audioapi::AudioNodeHostObject(
          graph,
          std::make_unique<WorkletNode>(
              context,
              UIWorkletsRunner(uiRuntime, uiScheduler, serializableWorklet),
              domain,
              options)),
      workletNode_(audioapi::typedAudioNode<WorkletNode>(node_)) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(WorkletNodeHostObject, bufferLength),
      JSI_EXPORT_PROPERTY_GETTER(WorkletNodeHostObject, smoothingTimeConstant));

  addSetters(JSI_EXPORT_PROPERTY_SETTER(WorkletNodeHostObject, smoothingTimeConstant));
}

size_t WorkletNodeHostObject::getMemoryPressure() const {
  return AudioNodeHostObject::getMemoryPressure() + workletNode_->getBufferLength() * sizeof(float);
}

JSI_PROPERTY_GETTER_IMPL(WorkletNodeHostObject, bufferLength) {
  return {static_cast<double>(workletNode_->getBufferLength())};
}

JSI_PROPERTY_GETTER_IMPL(WorkletNodeHostObject, smoothingTimeConstant) {
  return {workletNode_->getSmoothingTimeConstant()};
}

JSI_PROPERTY_SETTER_IMPL(WorkletNodeHostObject, smoothingTimeConstant) {
  workletNode_->setSmoothingTimeConstant(static_cast<float>(value.getNumber()));
}

} // namespace audioworklets
