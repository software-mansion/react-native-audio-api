#include <audioapi/HostObjects/sources/OscillatorNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/effects/PeriodicWaveHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/OscillatorNode.h>
#include <audioapi/types/NodeOptions.h>
#include <memory>
#include <utility>

namespace audioapi {

OscillatorNodeHostObject::OscillatorNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const OscillatorOptions &options)
    : AudioScheduledSourceNodeHostObject(context->createOscillator(options), options),
      type_(options.type) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
  frequencyParam_ = std::make_shared<AudioParamHostObject>(oscillatorNode->getFrequencyParam());
  detuneParam_ = std::make_shared<AudioParamHostObject>(oscillatorNode->getDetuneParam());

  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(OscillatorNodeHostObject, frequency),
      JSI_EXPORT_PROPERTY_GETTER(OscillatorNodeHostObject, detune),
      JSI_EXPORT_PROPERTY_GETTER(OscillatorNodeHostObject, type));

  addFunctions(JSI_EXPORT_FUNCTION(OscillatorNodeHostObject, setPeriodicWave));

  addSetters(JSI_EXPORT_PROPERTY_SETTER(OscillatorNodeHostObject, type));
}

JSI_PROPERTY_GETTER_IMPL(OscillatorNodeHostObject, frequency) {
  return jsi::Object::createFromHostObject(runtime, frequencyParam_);
}

JSI_PROPERTY_GETTER_IMPL(OscillatorNodeHostObject, detune) {
  return jsi::Object::createFromHostObject(runtime, detuneParam_);
}

JSI_PROPERTY_GETTER_IMPL(OscillatorNodeHostObject, type) {
  return jsi::String::createFromUtf8(runtime, js_enum_parser::oscillatorTypeToString(type_));
}

JSI_HOST_FUNCTION_IMPL(OscillatorNodeHostObject, setPeriodicWave) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
  auto periodicWave = args[0].getObject(runtime).getHostObject<PeriodicWaveHostObject>(runtime);

  auto event = [oscillatorNode, periodicWave = periodicWave->periodicWave_](BaseAudioContext &) {
    oscillatorNode->setPeriodicWave(periodicWave);
  };
  oscillatorNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_PROPERTY_SETTER_IMPL(OscillatorNodeHostObject, type) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
  auto type = js_enum_parser::oscillatorTypeFromString(value.asString(runtime).utf8(runtime));

  auto event = [oscillatorNode, type](BaseAudioContext &) {
    oscillatorNode->setType(type);
  };
  type_ = type;

  oscillatorNode->scheduleAudioEvent(std::move(event));
}

} // namespace audioapi
