#include <audioapi/HostObjects/effects/WaveShaperNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/WaveShaperNode.h>
#include <audioapi/utils/AudioArrayBuffer.hpp>

#include <memory>
#include <string>

namespace audioapi {

WaveShaperNodeHostObject::WaveShaperNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const WaveShaperOptions &options)
    : AudioNodeHostObject(context->createWaveShaper(options), options) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(WaveShaperNodeHostObject, oversample),
      JSI_EXPORT_PROPERTY_GETTER(WaveShaperNodeHostObject, curve));

  addSetters(JSI_EXPORT_PROPERTY_SETTER(WaveShaperNodeHostObject, oversample));
  addFunctions(JSI_EXPORT_FUNCTION(WaveShaperNodeHostObject, setCurve));
}

JSI_PROPERTY_GETTER_IMPL(WaveShaperNodeHostObject, oversample) {
  auto waveShaperNode = std::static_pointer_cast<WaveShaperNode>(node_);
  return jsi::String::createFromUtf8(
      runtime, js_enum_parser::overSampleTypeToString(waveShaperNode->getOversample()));
}

JSI_PROPERTY_GETTER_IMPL(WaveShaperNodeHostObject, curve) {
  auto waveShaperNode = std::static_pointer_cast<WaveShaperNode>(node_);
  auto curve = waveShaperNode->getCurve();

  if (curve == nullptr) {
    return jsi::Value::null();
  }

  // copy AudioArray holding curve data to avoid subsequent modifications
  auto audioArrayBuffer = std::make_shared<AudioArrayBuffer>(*curve);
  auto arrayBuffer = jsi::ArrayBuffer(runtime, audioArrayBuffer);

  auto float32ArrayCtor = runtime.global().getPropertyAsFunction(runtime, "Float32Array");
  auto float32Array = float32ArrayCtor.callAsConstructor(runtime, arrayBuffer).getObject(runtime);
  float32Array.setExternalMemoryPressure(runtime, audioArrayBuffer->size());

  return float32Array;
}

JSI_PROPERTY_SETTER_IMPL(WaveShaperNodeHostObject, oversample) {
  auto waveShaperNode = std::static_pointer_cast<WaveShaperNode>(node_);
  auto type = value.asString(runtime).utf8(runtime);
  waveShaperNode->setOversample(js_enum_parser::overSampleTypeFromString(type));
}

JSI_HOST_FUNCTION_IMPL(WaveShaperNodeHostObject, setCurve) {
  auto waveShaperNode = std::static_pointer_cast<WaveShaperNode>(node_);

  if (args[0].isNull()) {
    waveShaperNode->setCurve(nullptr);
    return jsi::Value::undefined();
  }

  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);

  auto curve = std::make_shared<AudioArrayBuffer>(
      reinterpret_cast<float *>(arrayBuffer.data(runtime)),
      static_cast<size_t>(arrayBuffer.size(runtime) / sizeof(float)));

  waveShaperNode->setCurve(curve);
  thisValue.asObject(runtime).setExternalMemoryPressure(runtime, arrayBuffer.size(runtime));

  return jsi::Value::undefined();
}

} // namespace audioapi
