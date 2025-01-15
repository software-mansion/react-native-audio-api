#pragma once

#include <memory>
#include <string>
#include <vector>

#include "AnalyserNode.h"
#include "AudioNodeHostObject.h"

namespace audioapi {
using namespace facebook;

class AnalyserNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AnalyserNodeHostObject(const std::shared_ptr<AnalyserNode> &node)
      : AudioNodeHostObject(node) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, fftSize),
        JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, frequencyBinCount),
        JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, minDecibels),
        JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, maxDecibels),
        JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, smoothingTimeConstant));

    addFunctions(
        JSI_EXPORT_FUNCTION(
            AnalyserNodeHostObject, getFloatFrequencyData),
        JSI_EXPORT_FUNCTION(
            AnalyserNodeHostObject, getByteFrequencyData),
        JSI_EXPORT_FUNCTION(
            AnalyserNodeHostObject, getFloatTimeDomainData),
        JSI_EXPORT_FUNCTION(
            AnalyserNodeHostObject, getByteTimeDomainData));

    addSetters(
        JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, fftSize),
        JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, minDecibels),
        JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, maxDecibels),
        JSI_EXPORT_PROPERTY_SETTER(
            AnalyserNodeHostObject, smoothingTimeConstant));
  }

  JSI_PROPERTY_GETTER(fftSize) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    return {analyserNode->getFftSize()};
  }

  JSI_PROPERTY_GETTER(frequencyBinCount) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    return {analyserNode->getFrequencyBinCount()};
  }

  JSI_PROPERTY_GETTER(minDecibels) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    return {analyserNode->getMinDecibels()};
  }

  JSI_PROPERTY_GETTER(maxDecibels) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    return {analyserNode->getMaxDecibels()};
  }

  JSI_PROPERTY_GETTER(smoothingTimeConstant) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    return {analyserNode->getSmoothingTimeConstant()};
  }

  JSI_HOST_FUNCTION(getFloatFrequencyData) {
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(getByteFrequencyData) {
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(getFloatTimeDomainData) {
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(getByteTimeDomainData) {
    return jsi::Value::undefined();
  }

  JSI_PROPERTY_SETTER(fftSize) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    auto fftSize = static_cast<int>(value.getNumber());
    analyserNode->setFftSize(fftSize);
  }

  JSI_PROPERTY_SETTER(minDecibels) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    auto minDecibels = static_cast<int>(value.getNumber());
    analyserNode->setMinDecibels(minDecibels);
  }

  JSI_PROPERTY_SETTER(maxDecibels) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
      auto maxDecibels = static_cast<int>(value.getNumber());
      analyserNode->setMaxDecibels(maxDecibels);
  }

  JSI_PROPERTY_SETTER(smoothingTimeConstant) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    auto smoothingTimeConstant = static_cast<int>(value.getNumber());
    analyserNode->setSmoothingTimeConstant(smoothingTimeConstant);
  }
};
} // namespace audioapi
