#include <audioworklets/HostObjects/RecorderWorkletHostObject.h>

#include <memory>

namespace audioworklets {

RecorderWorkletHostObject::RecorderWorkletHostObject(
    const std::shared_ptr<RecorderWorklet> &worklet)
    : audioapi::AudioConsumerHostObject(worklet), worklet_(worklet.get()) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(RecorderWorkletHostObject, bufferLength),
      JSI_EXPORT_PROPERTY_GETTER(RecorderWorkletHostObject, smoothingTimeConstant));

  addSetters(JSI_EXPORT_PROPERTY_SETTER(RecorderWorkletHostObject, smoothingTimeConstant));
}

JSI_PROPERTY_GETTER_IMPL(RecorderWorkletHostObject, bufferLength) {
  return {static_cast<double>(worklet_->snapshots().getBufferLength())};
}

JSI_PROPERTY_GETTER_IMPL(RecorderWorkletHostObject, smoothingTimeConstant) {
  return {worklet_->snapshots().getSmoothingTimeConstant()};
}

JSI_PROPERTY_SETTER_IMPL(RecorderWorkletHostObject, smoothingTimeConstant) {
  worklet_->snapshots().setSmoothingTimeConstant(static_cast<float>(value.getNumber()));
}

} // namespace audioworklets
