#include <audioapi/HostObjects/analysis/AnalyserNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {

AnalyserNodeHostObject::AnalyserNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const AnalyserOptions &options)
    : AudioNodeHostObject(
          context->getGraph(),
          std::make_unique<AnalyserNode>(context, options),
          options) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, fftSize),
      JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, minDecibels),
      JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, maxDecibels),
      JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, smoothingTimeConstant));

  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, fftSize),
      JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, minDecibels),
      JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, maxDecibels),
      JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, smoothingTimeConstant));

  addFunctions(
      JSI_EXPORT_FUNCTION(AnalyserNodeHostObject, getFloatFrequencyData),
      JSI_EXPORT_FUNCTION(AnalyserNodeHostObject, getByteFrequencyData),
      JSI_EXPORT_FUNCTION(AnalyserNodeHostObject, getFloatTimeDomainData),
      JSI_EXPORT_FUNCTION(AnalyserNodeHostObject, getByteTimeDomainData));
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, fftSize) {
  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  return {analyserNode->getFFTSize()};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, minDecibels) {
  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  return {analyserNode->getMinDecibels()};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, maxDecibels) {
  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  return {analyserNode->getMaxDecibels()};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, smoothingTimeConstant) {
  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  return {analyserNode->getSmoothingTimeConstant()};
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, fftSize) {
  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  analyserNode->setFFTSize(static_cast<int>(value.getNumber()));
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, minDecibels) {
  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  analyserNode->setMinDecibels(static_cast<float>(value.getNumber()));
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, maxDecibels) {
  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  analyserNode->setMaxDecibels(static_cast<float>(value.getNumber()));
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, smoothingTimeConstant) {
  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  analyserNode->setSmoothingTimeConstant(static_cast<float>(value.getNumber()));
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getFloatFrequencyData) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto data = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime) / sizeof(float));

  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  analyserNode->getFloatFrequencyData(data, length);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getByteFrequencyData) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto data = arrayBuffer.data(runtime);
  auto length = static_cast<int>(arrayBuffer.size(runtime));

  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  analyserNode->getByteFrequencyData(data, length);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getFloatTimeDomainData) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto data = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime) / sizeof(float));

  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  analyserNode->getFloatTimeDomainData(data, length);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getByteTimeDomainData) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto data = arrayBuffer.data(runtime);
  auto length = static_cast<int>(arrayBuffer.size(runtime));

  auto analyserNode = static_cast<AnalyserNode *>(node_->handle->audioNode->asAudioNode());
  analyserNode->getByteTimeDomainData(data, length);

  return jsi::Value::undefined();
}

} // namespace audioapi
