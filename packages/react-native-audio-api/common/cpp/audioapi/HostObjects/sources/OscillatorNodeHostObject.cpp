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
    : AudioScheduledSourceNodeHostObject(
          context->getGraph(),
          std::make_unique<OscillatorNode>(context, options),
          options),
      type_(options.type) {
  auto oscillatorNode = static_cast<OscillatorNode *>(node_->handle->audioNode->asAudioNode());
  frequencyParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, oscillatorNode->getFrequencyParam());
  detuneParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, oscillatorNode->getDetuneParam());

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
  auto handle = node_->handle;
  auto oscillatorNode = static_cast<OscillatorNode *>(handle->audioNode->asAudioNode());
  auto periodicWave = args[0].getObject(runtime).getHostObject<PeriodicWaveHostObject>(runtime);

  auto event = [handle, periodicWave = periodicWave->periodicWave_](BaseAudioContext &) {
    static_cast<OscillatorNode *>(handle->audioNode->asAudioNode())->setPeriodicWave(periodicWave);
  };
  oscillatorNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

JSI_PROPERTY_SETTER_IMPL(OscillatorNodeHostObject, type) {
  auto handle = node_->handle;
  auto oscillatorNode = static_cast<OscillatorNode *>(handle->audioNode->asAudioNode());
  auto type = js_enum_parser::oscillatorTypeFromString(value.asString(runtime).utf8(runtime));

  auto event = [handle, type](BaseAudioContext &) {
    static_cast<OscillatorNode *>(handle->audioNode->asAudioNode())->setType(type);
  };
  type_ = type;

  oscillatorNode->scheduleAudioEvent(std::move(event));
}

} // namespace audioapi
