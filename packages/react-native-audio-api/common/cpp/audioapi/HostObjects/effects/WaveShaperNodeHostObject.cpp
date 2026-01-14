#include <audioapi/HostObjects/effects/WaveShaperNodeHostObject.h>
#include <audioapi/core/effects/WaveShaperNode.h>
#include <audioapi/jsi/AudioArrayBuffer.h>

#include <memory>
#include <string>

namespace audioapi {

WaveShaperNodeHostObject::WaveShaperNodeHostObject(const std::shared_ptr<WaveShaperNode> &node)
    : AudioNodeHostObject(node) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(WaveShaperNodeHostObject, oversample),
      JSI_EXPORT_PROPERTY_GETTER(WaveShaperNodeHostObject, curve));

  addSetters(JSI_EXPORT_PROPERTY_SETTER(WaveShaperNodeHostObject, oversample));
  addFunctions(JSI_EXPORT_FUNCTION(WaveShaperNodeHostObject, setCurve));
}

JSI_PROPERTY_GETTER_IMPL(WaveShaperNodeHostObject, oversample) {
  auto waveShaperNode = std::static_pointer_cast<WaveShaperNode>(node_);
  return jsi::String::createFromUtf8(runtime, waveShaperNode->getOversample());
}

JSI_PROPERTY_GETTER_IMPL(WaveShaperNodeHostObject, curve) {
  auto waveShaperNode = std::static_pointer_cast<WaveShaperNode>(node_);
  auto curve = waveShaperNode->getCurve();

  if (curve == nullptr) {
    return jsi::Value::null();
  }

  // copy AudioArray holding curve data to avoid subsequent modifications
  auto audioArray = std::make_shared<AudioArray>(*curve);
  auto audioArrayBuffer = std::make_shared<AudioArrayBuffer>(audioArray);
  auto arrayBuffer = jsi::ArrayBuffer(runtime, audioArrayBuffer);

  auto float32ArrayCtor = runtime.global().getPropertyAsFunction(runtime, "Float32Array");
  auto float32Array = float32ArrayCtor.callAsConstructor(runtime, arrayBuffer).getObject(runtime);
  float32Array.setExternalMemoryPressure(runtime, audioArrayBuffer->size());

  return float32Array;
}

JSI_PROPERTY_SETTER_IMPL(WaveShaperNodeHostObject, oversample) {
  auto waveShaperNode = std::static_pointer_cast<WaveShaperNode>(node_);
  std::string type = value.asString(runtime).utf8(runtime);
  waveShaperNode->setOversample(type);
}

JSI_HOST_FUNCTION_IMPL(WaveShaperNodeHostObject, setCurve) {
  auto waveShaperNode = std::static_pointer_cast<WaveShaperNode>(node_);

  if (args[0].isNull()) {
    waveShaperNode->setCurve(std::shared_ptr<AudioArray>(nullptr));
    return jsi::Value::undefined();
  }

  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);

  auto curve = std::make_shared<AudioArray>(
      reinterpret_cast<float *>(arrayBuffer.data(runtime)),
      static_cast<size_t>(arrayBuffer.size(runtime) / sizeof(float)));

  waveShaperNode->setCurve(curve);
  thisValue.asObject(runtime).setExternalMemoryPressure(runtime, arrayBuffer.size(runtime));

  return jsi::Value::undefined();
}

} // namespace audioapi
