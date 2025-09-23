#include <audioapi/HostObjects/sources/OscillatorNodeHostObject.h>

namespace audioapi {

JSI_PROPERTY_GETTER_IMPL(OscillatorNodeHostObject, frequency) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
  auto frequencyParam_ = std::make_shared<AudioParamHostObject>(
      oscillatorNode->getFrequencyParam());
  return jsi::Object::createFromHostObject(runtime, frequencyParam_);
}

JSI_PROPERTY_GETTER_IMPL(OscillatorNodeHostObject, detune) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
  auto detuneParam_ = std::make_shared<AudioParamHostObject>(
      oscillatorNode->getDetuneParam());
  return jsi::Object::createFromHostObject(runtime, detuneParam_);
}

JSI_PROPERTY_GETTER_IMPL(OscillatorNodeHostObject, type) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
  auto waveType = oscillatorNode->getType();
  return jsi::String::createFromUtf8(runtime, waveType);
}

JSI_HOST_FUNCTION_IMPL(OscillatorNodeHostObject, setPeriodicWave) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
  auto periodicWave =
      args[0].getObject(runtime).getHostObject<PeriodicWaveHostObject>(
          runtime);
  oscillatorNode->setPeriodicWave(periodicWave->periodicWave_);
  return jsi::Value::undefined();
}

JSI_PROPERTY_SETTER_IMPL(OscillatorNodeHostObject, type) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
  oscillatorNode->setType(value.getString(runtime).utf8(runtime));
}

} // namespace audioapi