#include <audioapi/HostObjects/effects/BiquadFilterNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/effects/BiquadFilterNode.h>
#include <memory>

namespace audioapi {

BiquadFilterNodeHostObject::BiquadFilterNodeHostObject(
    const std::shared_ptr<BiquadFilterNode> &node)
    : AudioNodeHostObject(node) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, frequency),
      JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, detune),
      JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, Q),
      JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, gain),
      JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, type));

  addSetters(JSI_EXPORT_PROPERTY_SETTER(BiquadFilterNodeHostObject, type));

  addFunctions(JSI_EXPORT_FUNCTION(BiquadFilterNodeHostObject, getFrequencyResponse));
}

JSI_PROPERTY_GETTER_IMPL(BiquadFilterNodeHostObject, frequency) {
  auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
  auto frequencyParam_ =
      std::make_shared<AudioParamHostObject>(biquadFilterNode->getFrequencyParam());
  return jsi::Object::createFromHostObject(runtime, frequencyParam_);
}

JSI_PROPERTY_GETTER_IMPL(BiquadFilterNodeHostObject, detune) {
  auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
  auto detuneParam_ = std::make_shared<AudioParamHostObject>(biquadFilterNode->getDetuneParam());
  return jsi::Object::createFromHostObject(runtime, detuneParam_);
}

JSI_PROPERTY_GETTER_IMPL(BiquadFilterNodeHostObject, Q) {
  auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
  auto QParam_ = std::make_shared<AudioParamHostObject>(biquadFilterNode->getQParam());
  return jsi::Object::createFromHostObject(runtime, QParam_);
}

JSI_PROPERTY_GETTER_IMPL(BiquadFilterNodeHostObject, gain) {
  auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
  auto gainParam_ = std::make_shared<AudioParamHostObject>(biquadFilterNode->getGainParam());
  return jsi::Object::createFromHostObject(runtime, gainParam_);
}

JSI_PROPERTY_GETTER_IMPL(BiquadFilterNodeHostObject, type) {
  auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
  auto type = biquadFilterNode->getType();
  return jsi::String::createFromUtf8(runtime, type);
}

JSI_PROPERTY_SETTER_IMPL(BiquadFilterNodeHostObject, type) {
  auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
  biquadFilterNode->setType(value.getString(runtime).utf8(runtime));
}

JSI_HOST_FUNCTION_IMPL(BiquadFilterNodeHostObject, getFrequencyResponse) {
  auto arrayBufferFrequency =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto frequencyArray = reinterpret_cast<float *>(arrayBufferFrequency.data(runtime));
  auto length = static_cast<size_t>(arrayBufferFrequency.size(runtime));

  auto arrayBufferMag =
      args[1].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto magResponseOut = reinterpret_cast<float *>(arrayBufferMag.data(runtime));

  auto arrayBufferPhase =
      args[2].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto phaseResponseOut = reinterpret_cast<float *>(arrayBufferPhase.data(runtime));

  auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
  biquadFilterNode->getFrequencyResponse(frequencyArray, magResponseOut, phaseResponseOut, length);

  return jsi::Value::undefined();
}

} // namespace audioapi
