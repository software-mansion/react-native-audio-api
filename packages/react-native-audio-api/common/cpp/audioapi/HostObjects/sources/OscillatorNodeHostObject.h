#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/types/OscillatorType.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class OscillatorNode;

class OscillatorNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit OscillatorNodeHostObject(const std::shared_ptr<OscillatorNode> &node);

  JSI_PROPERTY_GETTER_DECL(frequency);
  JSI_PROPERTY_GETTER_DECL(detune);
  JSI_PROPERTY_GETTER_DECL(type);

  JSI_HOST_FUNCTION_DECL(setPeriodicWave);

  JSI_PROPERTY_SETTER_DECL(type);

  static OscillatorType oscillatorTypeFromString(const std::string &type) {
    if (type == "sine")
      return OscillatorType::SINE;
    if (type == "square")
      return OscillatorType::SQUARE;
    if (type == "sawtooth")
      return OscillatorType::SAWTOOTH;
    if (type == "triangle")
      return OscillatorType::TRIANGLE;
    if (type == "custom")
      return OscillatorType::CUSTOM;

    throw std::invalid_argument("Unknown oscillator type: " + type);
  }

  static std::string oscillatorTypeToString(const OscillatorType type) {
    switch (type) {
      case OscillatorType::SINE:
        return "sine";
      case OscillatorType::SQUARE:
        return "square";
      case OscillatorType::SAWTOOTH:
        return "sawtooth";
      case OscillatorType::TRIANGLE:
        return "triangle";
      case OscillatorType::CUSTOM:
        return "custom";
      default:
        throw std::invalid_argument("Unknown oscillator type");
    }
  }
};
} // namespace audioapi
