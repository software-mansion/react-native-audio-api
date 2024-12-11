#include "BiquadFilterNodeHostObject.h"

namespace audioapi {
using namespace facebook;

std::shared_ptr<BiquadFilterNode>
BiquadFilterNodeHostObject::getBiquadFilterNodeFromAudioNode() {
  return std::static_pointer_cast<BiquadFilterNode>(node_);
}

BiquadFilterNodeHostObject::BiquadFilterNodeHostObject(
    const std::shared_ptr<BiquadFilterNode> &node)
    : AudioNodeHostObject(node) {
  frequencyParam_ =
      std::make_shared<AudioParamHostObject>(node->getFrequencyParam());
  detuneParam_ = std::make_shared<AudioParamHostObject>(node->getDetuneParam());
  QParam_ = std::make_shared<AudioParamHostObject>(node->getQParam());
  gainParam_ = std::make_shared<AudioParamHostObject>(node->getGainParam());
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
  propertyNames.push_back(
      jsi::PropNameID::forAscii(runtime, "getFrequencyResponse"));
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
    auto node = getBiquadFilterNodeFromAudioNode();
    auto waveType = node->getType();
    return jsi::String::createFromUtf8(runtime, waveType);
  }

  if (propName == "getFrequencyResponse") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        3,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisVal,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto frequencyArray = args[0].getObject(rt).asArray(rt);
          auto magResponseOut = args[1].getObject(rt).asArray(rt);
          auto phaseResponseOut = args[2].getObject(rt).asArray(rt);

          std::vector<float> frequencyArrayVector(frequencyArray.length(rt));
          for (size_t i = 0; i < frequencyArray.length(rt); i++) {
            frequencyArrayVector[i] = static_cast<float>(
                frequencyArray.getValueAtIndex(rt, i).getNumber());
          }

          std::vector<float> magResponseOutVector(magResponseOut.length(rt));
          std::vector<float> phaseResponseOutVector(
              phaseResponseOut.length(rt));

          auto node = getBiquadFilterNodeFromAudioNode();
          node->getFrequencyResponse(
              frequencyArrayVector,
              magResponseOutVector,
              phaseResponseOutVector);

          for (size_t i = 0; i < magResponseOutVector.size(); i++) {
            magResponseOut.setValueAtIndex(rt, i, magResponseOutVector[i]);
          }

          for (size_t i = 0; i < phaseResponseOutVector.size(); i++) {
            phaseResponseOut.setValueAtIndex(rt, i, phaseResponseOutVector[i]);
          }

          return jsi::Value::undefined();
        });
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
    auto node = getBiquadFilterNodeFromAudioNode();
    node->setType(filterType);
    return;
  }

  return AudioNodeHostObject::set(runtime, propNameId, value);
}
} // namespace audioapi
