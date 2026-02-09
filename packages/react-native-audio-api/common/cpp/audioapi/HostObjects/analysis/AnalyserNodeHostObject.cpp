#include <audioapi/HostObjects/analysis/AnalyserNodeHostObject.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/analysis/AnalyserNode.h>

#include <memory>
#include <utility>

namespace audioapi {

AnalyserNodeHostObject::AnalyserNodeHostObject(const std::shared_ptr<BaseAudioContext>& context, const AnalyserOptions &options)
    : AudioNodeHostObject(context->createAnalyser(options), options),
      fftSize_(options.fftSize),
      minDecibels_(options.minDecibels),
      maxDecibels_(options.maxDecibels),
      smoothingTimeConstant_(options.smoothingTimeConstant),
      windowType_(options.windowType) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, fftSize),
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
  return {fftSize_};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, minDecibels) {
  return {minDecibels_};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, maxDecibels) {
  return {maxDecibels_};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, smoothingTimeConstant) {
  return {smoothingTimeConstant_};
}

JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, window) {
  return jsi::String::createFromUtf8(runtime, js_enum_parser::windowTypeToString(windowType_));
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, fftSize) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);

  auto fftSize = static_cast<int>(value.getNumber());
  auto event = [analyserNode, fftSize](BaseAudioContext&) {
    analyserNode->setFftSize(fftSize);
  };
  analyserNode->scheduleAudioEvent(std::move(event));
  fftSize_ = fftSize;
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, minDecibels) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  auto minDecibels = static_cast<float>(value.getNumber());
  auto event = [analyserNode, minDecibels](BaseAudioContext&) {
    analyserNode->setMinDecibels(minDecibels);
  };
    analyserNode->scheduleAudioEvent(std::move(event));
    minDecibels_ = minDecibels;
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, maxDecibels) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  auto maxDecibels = static_cast<float>(value.getNumber());
  auto event = [analyserNode, maxDecibels](BaseAudioContext&) {
    analyserNode->setMaxDecibels(maxDecibels);
  };
    analyserNode->scheduleAudioEvent(std::move(event));
    maxDecibels_ = maxDecibels;
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, smoothingTimeConstant) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  auto smoothingTimeConstant = static_cast<float>(value.getNumber());
    auto event = [analyserNode, smoothingTimeConstant](BaseAudioContext&) {
        analyserNode->setSmoothingTimeConstant(smoothingTimeConstant);
    };
        analyserNode->scheduleAudioEvent(std::move(event));
        smoothingTimeConstant_ = smoothingTimeConstant;
}

JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, window) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  auto windowType = js_enum_parser::windowTypeFromString(value.asString(runtime).utf8(runtime));
  auto event = [analyserNode, windowType](BaseAudioContext&) {
    analyserNode->setWindowType(windowType);
  };
  analyserNode->scheduleAudioEvent(std::move(event));
  windowType_ = windowType;
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getFloatFrequencyData) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto data = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime));

  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  analyserNode->getFloatFrequencyData(data, length);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getByteFrequencyData) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto data = arrayBuffer.data(runtime);
  auto length = static_cast<int>(arrayBuffer.size(runtime));

  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  analyserNode->getByteFrequencyData(data, length);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getFloatTimeDomainData) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto data = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime));

  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  analyserNode->getFloatTimeDomainData(data, length);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getByteTimeDomainData) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto data = arrayBuffer.data(runtime);
  auto length = static_cast<int>(arrayBuffer.size(runtime));

  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  analyserNode->getByteTimeDomainData(data, length);

  return jsi::Value::undefined();
}

} // namespace audioapi
