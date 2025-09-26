#include <audioapi/HostObjects/analysis/AnalyserNodeHostObject.h>

#include <audioapi/core/analysis/AnalyserNode.h>

namespace audioapi {

AnalyserNodeHostObject::AnalyserNodeHostObject(
    const std::shared_ptr<AnalyserNode> &node)
    : AudioNodeHostObject(node) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, fftSize),
      JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, frequencyBinCount),
      JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, minDecibels),
      JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, maxDecibels),
      JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, smoothingTimeConstant),
      JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, window));

  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, fftSize),
      JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, minDecibels),
      JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, maxDecibels),
      JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, smoothingTimeConstant),
      JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, window));

  addFunctions(
      JSI_EXPORT_FUNCTION(AnalyserNodeHostObject, getFloatFrequencyData),
      JSI_EXPORT_FUNCTION(AnalyserNodeHostObject, getByteFrequencyData),
      JSI_EXPORT_FUNCTION(AnalyserNodeHostObject, getFloatTimeDomainData),
      JSI_EXPORT_FUNCTION(AnalyserNodeHostObject, getByteTimeDomainData));
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, fftSize) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  return {static_cast<int>(analyserNode->getFftSize())};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, frequencyBinCount) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  return {static_cast<int>(analyserNode->getFrequencyBinCount())};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, minDecibels) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  return {analyserNode->getMinDecibels()};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, maxDecibels) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  return {analyserNode->getMaxDecibels()};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, smoothingTimeConstant) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  return {analyserNode->getSmoothingTimeConstant()};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, window) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  auto windowType = analyserNode->getWindowType();
  return jsi::String::createFromUtf8(runtime, windowType);
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, fftSize) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  auto fftSize = static_cast<int>(value.getNumber());
  analyserNode->setFftSize(fftSize);
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, minDecibels) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  auto minDecibels = static_cast<float>(value.getNumber());
  analyserNode->setMinDecibels(minDecibels);
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, maxDecibels) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  auto maxDecibels = static_cast<float>(value.getNumber());
  analyserNode->setMaxDecibels(maxDecibels);
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, smoothingTimeConstant) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  auto smoothingTimeConstant = static_cast<float>(value.getNumber());
  analyserNode->setSmoothingTimeConstant(smoothingTimeConstant);
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, window) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  analyserNode->setWindowType(value.getString(runtime).utf8(runtime));
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getFloatFrequencyData) {
  auto arrayBuffer = args[0]
                         .getObject(runtime)
                         .getPropertyAsObject(runtime, "buffer")
                         .getArrayBuffer(runtime);
  auto data = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime));

  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  analyserNode->getFloatFrequencyData(data, length);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getByteFrequencyData) {
  auto arrayBuffer = args[0]
                         .getObject(runtime)
                         .getPropertyAsObject(runtime, "buffer")
                         .getArrayBuffer(runtime);
  auto data = arrayBuffer.data(runtime);
  auto length = static_cast<int>(arrayBuffer.size(runtime));

  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  analyserNode->getByteFrequencyData(data, length);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getFloatTimeDomainData) {
  auto arrayBuffer = args[0]
                         .getObject(runtime)
                         .getPropertyAsObject(runtime, "buffer")
                         .getArrayBuffer(runtime);
  auto data = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime));

  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  analyserNode->getFloatTimeDomainData(data, length);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getByteTimeDomainData) {
  auto arrayBuffer = args[0]
                         .getObject(runtime)
                         .getPropertyAsObject(runtime, "buffer")
                         .getArrayBuffer(runtime);
  auto data = arrayBuffer.data(runtime);
  auto length = static_cast<int>(arrayBuffer.size(runtime));

  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  analyserNode->getByteTimeDomainData(data, length);

  return jsi::Value::undefined();
}

} // namespace audioapi
