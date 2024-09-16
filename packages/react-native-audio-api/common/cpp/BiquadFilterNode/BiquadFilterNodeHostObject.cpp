#include "BiquadFilterNodeHostObject.h"

namespace audioapi {
using namespace facebook;

std::shared_ptr<BiquadFilterNodeWrapper>
BiquadFilterNodeHostObject::getBiquadFilterNodeWrapperFromAudioNodeWrapper() {
  return std::static_pointer_cast<BiquadFilterNodeWrapper>(wrapper_);
}

BiquadFilterNodeHostObject::BiquadFilterNodeHostObject(
    const std::shared_ptr<BiquadFilterNodeWrapper> &wrapper)
    : AudioNodeHostObject(wrapper) {
  auto frequencyParam = wrapper->getFrequencyParam();
  frequencyParam_ = AudioParamHostObject::createFromWrapper(frequencyParam);
  auto detuneParam = wrapper->getDetuneParam();
  detuneParam_ = AudioParamHostObject::createFromWrapper(detuneParam);
  auto QParam = wrapper->getQParam();
  QParam_ = AudioParamHostObject::createFromWrapper(QParam);
  auto gainParam = wrapper->getGainParam();
  gainParam_ = AudioParamHostObject::createFromWrapper(gainParam);
}

std::vector<jsi::PropNameID> BiquadFilterNodeHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames =
      AudioNodeHostObject::getPropertyNames(runtime);
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "frequency"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "detune"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "Q"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "gain"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "type"));
  return propertyNames;
}

jsi::Value BiquadFilterNodeHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "frequency") {
    return jsi::Object::createFromHostObject(runtime, frequencyParam_);
  }

  if (propName == "detune") {
    return jsi::Object::createFromHostObject(runtime, detuneParam_);
  }

  if (propName == "Q") {
    return jsi::Object::createFromHostObject(runtime, QParam_);
  }

  if (propName == "gain") {
    return jsi::Object::createFromHostObject(runtime, gainParam_);
  }

  if (propName == "type") {
    auto wrapper = getBiquadFilterNodeWrapperFromAudioNodeWrapper();
    auto waveType = wrapper->getType();
    return jsi::String::createFromUtf8(runtime, waveType);
  }

  return AudioNodeHostObject::get(runtime, propNameId);
}

void BiquadFilterNodeHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "type") {
    std::string filterType = value.getString(runtime).utf8(runtime);
    auto wrapper = getBiquadFilterNodeWrapperFromAudioNodeWrapper();
    wrapper->setType(filterType);
    return;
  }

  throw std::runtime_error("Not yet implemented!");
}
} // namespace audioapi
