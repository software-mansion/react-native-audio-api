#pragma once

#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <string>
#include <vector>

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
        JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, smoothingTimeConstant),
        JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, window));

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
            AnalyserNodeHostObject, smoothingTimeConstant),
        JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, window));
  }

  JSI_PROPERTY_GETTER(fftSize) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    return {static_cast<int>(analyserNode->getFftSize())};
  }

  JSI_PROPERTY_GETTER(frequencyBinCount) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    return {static_cast<int>(analyserNode->getFrequencyBinCount())};
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

  JSI_PROPERTY_GETTER(window) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    auto windowType = analyserNode->getWindowType();
    return jsi::String::createFromUtf8(runtime, windowType);
  }

  JSI_HOST_FUNCTION(getFloatFrequencyData) {
    auto destination = args[0].getObject(runtime).asArray(runtime);
    auto length = static_cast<int>(destination.getProperty(runtime, "length").asNumber());
    auto data = new float[length];

    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
      analyserNode->getFloatFrequencyData(data, length);

    for (int i = 0; i < length; i++) {
        destination.setValueAtIndex(runtime, i, jsi::Value(data[i]));
    }

    delete[] data;
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(getByteFrequencyData) {
    auto destination = args[0].getObject(runtime).asArray(runtime);
    auto length = static_cast<int>(destination.getProperty(runtime, "length").asNumber());
    auto data = new uint8_t[length];

    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    analyserNode->getByteFrequencyData(data, length);

    for (int i = 0; i < length; i++) {
        destination.setValueAtIndex(runtime, i, jsi::Value(data[i]));
    }

    delete[] data;
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(getFloatTimeDomainData) {
    auto destination = args[0].getObject(runtime).asArray(runtime);
    auto length = static_cast<int>(destination.getProperty(runtime, "length").asNumber());
    auto data = new float[length];

    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    analyserNode->getFloatTimeDomainData(data, length);

    for (int i = 0; i < length; i++) {
        destination.setValueAtIndex(runtime, i, jsi::Value(data[i]));
    }

    delete[] data;
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(getByteTimeDomainData) {
      auto destination = args[0].getObject(runtime).asArray(runtime);
      auto length = static_cast<int>(destination.getProperty(runtime, "length").asNumber());
      auto data = new uint8_t[length];

      auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
      analyserNode->getByteTimeDomainData(data, length);

      for (int i = 0; i < length; i++) {
          destination.setValueAtIndex(runtime, i, jsi::Value(data[i]));
      }

      delete[] data;
      return jsi::Value::undefined();
  }

  JSI_PROPERTY_SETTER(fftSize) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    auto fftSize = static_cast<int>(value.getNumber());
    analyserNode->setFftSize(fftSize);
  }

  JSI_PROPERTY_SETTER(minDecibels) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    auto minDecibels = static_cast<float>(value.getNumber());
    analyserNode->setMinDecibels(minDecibels);
  }

  JSI_PROPERTY_SETTER(maxDecibels) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
      auto maxDecibels = static_cast<float>(value.getNumber());
      analyserNode->setMaxDecibels(maxDecibels);
  }

  JSI_PROPERTY_SETTER(smoothingTimeConstant) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    auto smoothingTimeConstant = static_cast<float>(value.getNumber());
    analyserNode->setSmoothingTimeConstant(smoothingTimeConstant);
  }

  JSI_PROPERTY_SETTER(window) {
    auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
    analyserNode->setWindowType(value.getString(runtime).utf8(runtime));
  }
};
} // namespace audioapi
