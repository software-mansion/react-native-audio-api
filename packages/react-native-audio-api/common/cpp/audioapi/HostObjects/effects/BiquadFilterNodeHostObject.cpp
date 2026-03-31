#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/effects/BiquadFilterNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/BiquadFilterNode.h>
#include <audioapi/core/types/BiquadFilterType.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <utility>

namespace audioapi {

BiquadFilterNodeHostObject::BiquadFilterNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const BiquadFilterOptions &options)
    : AudioNodeHostObject(context->createBiquadFilter(options), options), type_(options.type) {
  auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
  frequencyParam_ = std::make_shared<AudioParamHostObject>(biquadFilterNode->getFrequencyParam());
  detuneParam_ = std::make_shared<AudioParamHostObject>(biquadFilterNode->getDetuneParam());
  QParam_ = std::make_shared<AudioParamHostObject>(biquadFilterNode->getQParam());
  gainParam_ = std::make_shared<AudioParamHostObject>(biquadFilterNode->getGainParam());

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
  return jsi::Object::createFromHostObject(runtime, frequencyParam_);
}

JSI_PROPERTY_GETTER_IMPL(BiquadFilterNodeHostObject, detune) {
  return jsi::Object::createFromHostObject(runtime, detuneParam_);
}

JSI_PROPERTY_GETTER_IMPL(BiquadFilterNodeHostObject, Q) {
  return jsi::Object::createFromHostObject(runtime, QParam_);
}

JSI_PROPERTY_GETTER_IMPL(BiquadFilterNodeHostObject, gain) {
  return jsi::Object::createFromHostObject(runtime, gainParam_);
}

JSI_PROPERTY_GETTER_IMPL(BiquadFilterNodeHostObject, type) {
  return jsi::String::createFromUtf8(runtime, js_enum_parser::filterTypeToString(type_));
}

JSI_PROPERTY_SETTER_IMPL(BiquadFilterNodeHostObject, type) {
  auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);

  auto type = js_enum_parser::filterTypeFromString(value.asString(runtime).utf8(runtime));
  auto event = [biquadFilterNode, type](BaseAudioContext &) {
    biquadFilterNode->setType(type);
  };
  biquadFilterNode->scheduleAudioEvent(std::move(event));
  type_ = type;
}

JSI_HOST_FUNCTION_IMPL(BiquadFilterNodeHostObject, getFrequencyResponse) {
  auto arrayBufferFrequency =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto frequencyArray = reinterpret_cast<float *>(arrayBufferFrequency.data(runtime));
  auto length = static_cast<size_t>(arrayBufferFrequency.size(runtime) / sizeof(float));

  auto arrayBufferMag =
      args[1].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto magResponseOut = reinterpret_cast<float *>(arrayBufferMag.data(runtime));

  auto arrayBufferPhase =
      args[2].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto phaseResponseOut = reinterpret_cast<float *>(arrayBufferPhase.data(runtime));

  auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
  biquadFilterNode->getFrequencyResponse(
      frequencyArray, magResponseOut, phaseResponseOut, length, type_);

  return jsi::Value::undefined();
}

} // namespace audioapi
