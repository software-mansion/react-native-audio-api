#include "OscillatorNodeHostObject.h"

namespace audioapi {
using namespace facebook;

std::shared_ptr<OscillatorNodeWrapper>
OscillatorNodeHostObject::getOscillatorNodeWrapperFromAudioNodeWrapper() {
  return std::static_pointer_cast<OscillatorNodeWrapper>(wrapper_);
}

OscillatorNodeHostObject::OscillatorNodeHostObject(
    const std::shared_ptr<OscillatorNodeWrapper> &wrapper)
    : AudioScheduledSourceNodeHostObject(wrapper) {
  auto frequencyParam = wrapper->getFrequencyParam();
  frequencyParam_ = AudioParamHostObject::createFromWrapper(frequencyParam);
  auto detuneParam = wrapper->getDetuneParam();
  detuneParam_ = AudioParamHostObject::createFromWrapper(detuneParam);
}

std::vector<jsi::PropNameID> OscillatorNodeHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames =
          AudioScheduledSourceNodeHostObject::getPropertyNames(runtime);
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "frequency"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "detune"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "type"));
  return propertyNames;
}

jsi::Value OscillatorNodeHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "frequency") {
    return jsi::Object::createFromHostObject(runtime, frequencyParam_);
  }

  if (propName == "detune") {
    return jsi::Object::createFromHostObject(runtime, detuneParam_);
  }

  if (propName == "type") {
    auto wrapper = getOscillatorNodeWrapperFromAudioNodeWrapper();
    auto waveType = wrapper->getType();
    return jsi::String::createFromUtf8(runtime, waveType);
  }

  return AudioScheduledSourceNodeHostObject::get(runtime, propNameId);
}

void OscillatorNodeHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "type") {
    std::string waveType = value.getString(runtime).utf8(runtime);
    auto wrapper = getOscillatorNodeWrapperFromAudioNodeWrapper();
    wrapper->setType(waveType);
    return;
  }

  return AudioScheduledSourceNodeHostObject::set(runtime,propNameId,value);
}
} // namespace audioapi
