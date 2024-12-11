#include "OscillatorNodeHostObject.h"

namespace audioapi {
using namespace facebook;

std::shared_ptr<OscillatorNode>
OscillatorNodeHostObject::getOscillatorNodeFromAudioNode() {
  return std::static_pointer_cast<OscillatorNode>(node_);
}

OscillatorNodeHostObject::OscillatorNodeHostObject(
    const std::shared_ptr<OscillatorNode> &node)
    : AudioScheduledSourceNodeHostObject(node) {
  auto frequencyParam = node->getFrequencyParam();
  frequencyParam_ = std::make_shared<AudioParamHostObject>(frequencyParam);
  auto detuneParam = node->getDetuneParam();
  detuneParam_ = std::make_shared<AudioParamHostObject>(detuneParam);
}

std::vector<jsi::PropNameID> OscillatorNodeHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames =
      AudioScheduledSourceNodeHostObject::getPropertyNames(runtime);
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "frequency"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "detune"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "type"));
  propertyNames.push_back(
      jsi::PropNameID::forAscii(runtime, "setPeriodicWave"));
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
    auto node = getOscillatorNodeFromAudioNode();
    auto waveType = node->getType();
    return jsi::String::createFromUtf8(runtime, waveType);
  }

  if (propName == "setPeriodicWave") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        1,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisVal,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto node = getOscillatorNodeFromAudioNode();
          auto periodicWaveHostObject =
              args[0].getObject(rt).asHostObject<PeriodicWaveHostObject>(rt);

          node->setPeriodicWave(periodicWaveHostObject->periodicWave_);
          return jsi::Value::undefined();
        });
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
    auto node = getOscillatorNodeFromAudioNode();
    node->setType(waveType);
    return;
  }

  return AudioScheduledSourceNodeHostObject::set(runtime, propNameId, value);
}
} // namespace audioapi
