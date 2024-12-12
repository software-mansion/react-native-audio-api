#pragma once

#include <memory>
#include <string>
#include <vector>

#include "AudioNodeHostObject.h"
#include "AudioParamHostObject.h"
#include "BiquadFilterNode.h"

namespace audioapi {
using namespace facebook;

class BiquadFilterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit BiquadFilterNodeHostObject(
      const std::shared_ptr<BiquadFilterNode> &node)
      : AudioNodeHostObject(node) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, frequency),
        JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, detune),
        JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, Q),
        JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, gain),
        JSI_EXPORT_PROPERTY_GETTER(BiquadFilterNodeHostObject, type));

    addFunctions(
        JSI_EXPORT_FUNCTION(BiquadFilterNodeHostObject, getFrequencyResponse));

    addSetters(JSI_EXPORT_PROPERTY_SETTER(BiquadFilterNodeHostObject, type));
  }

  JSI_PROPERTY_GETTER(frequency) {
    auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
    auto frequencyParam_ = std::make_shared<AudioParamHostObject>(
        biquadFilterNode->getFrequencyParam());
    return jsi::Object::createFromHostObject(runtime, frequencyParam_);
  }

  JSI_PROPERTY_GETTER(detune) {
    auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
    auto detuneParam_ = std::make_shared<AudioParamHostObject>(
        biquadFilterNode->getDetuneParam());
    return jsi::Object::createFromHostObject(runtime, detuneParam_);
  }

  JSI_PROPERTY_GETTER(Q) {
    auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
    auto QParam_ =
        std::make_shared<AudioParamHostObject>(biquadFilterNode->getQParam());
    return jsi::Object::createFromHostObject(runtime, QParam_);
  }

  JSI_PROPERTY_GETTER(gain) {
    auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
    auto gainParam_ = std::make_shared<AudioParamHostObject>(
        biquadFilterNode->getGainParam());
    return jsi::Object::createFromHostObject(runtime, gainParam_);
  }

  JSI_PROPERTY_GETTER(type) {
    auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
    auto type = biquadFilterNode->getType();
    return jsi::String::createFromUtf8(runtime, type);
  }

  JSI_HOST_FUNCTION(getFrequencyResponse) {
    auto frequencyArray = args[0].getObject(runtime).asArray(runtime);
    auto magResponseOut = args[1].getObject(runtime).asArray(runtime);
    auto phaseResponseOut = args[2].getObject(runtime).asArray(runtime);

    std::vector<float> frequencyArrayVector(frequencyArray.length(runtime));
    for (size_t i = 0; i < frequencyArray.length(runtime); i++) {
      frequencyArrayVector[i] = static_cast<float>(
          frequencyArray.getValueAtIndex(runtime, i).getNumber());
    }

    std::vector<float> magResponseOutVector(magResponseOut.length(runtime));
    std::vector<float> phaseResponseOutVector(phaseResponseOut.length(runtime));

    auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
    biquadFilterNode->getFrequencyResponse(
        frequencyArrayVector, magResponseOutVector, phaseResponseOutVector);

    for (size_t i = 0; i < magResponseOutVector.size(); i++) {
      magResponseOut.setValueAtIndex(runtime, i, magResponseOutVector[i]);
    }

    for (size_t i = 0; i < phaseResponseOutVector.size(); i++) {
      phaseResponseOut.setValueAtIndex(runtime, i, phaseResponseOutVector[i]);
    }

    return jsi::Value::undefined();
  }

  JSI_PROPERTY_SETTER(type) {
    auto biquadFilterNode = std::static_pointer_cast<BiquadFilterNode>(node_);
    biquadFilterNode->setType(value.getString(runtime).utf8(runtime));
  }
};
} // namespace audioapi
