#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/types/BiquadFilterType.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class BiquadFilterNode;

class BiquadFilterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit BiquadFilterNodeHostObject(const std::shared_ptr<BiquadFilterNode> &node);

  JSI_PROPERTY_GETTER_DECL(frequency);
  JSI_PROPERTY_GETTER_DECL(detune);
  JSI_PROPERTY_GETTER_DECL(Q);
  JSI_PROPERTY_GETTER_DECL(gain);
  JSI_PROPERTY_GETTER_DECL(type);

  JSI_PROPERTY_SETTER_DECL(type);

  JSI_HOST_FUNCTION_DECL(getFrequencyResponse);

  static BiquadFilterType filterTypeFromString(const std::string &type) {
    if (type == "lowpass")
      return BiquadFilterType::LOWPASS;
    if (type == "highpass")
      return BiquadFilterType::HIGHPASS;
    if (type == "bandpass")
      return BiquadFilterType::BANDPASS;
    if (type == "lowshelf")
      return BiquadFilterType::LOWSHELF;
    if (type == "highshelf")
      return BiquadFilterType::HIGHSHELF;
    if (type == "peaking")
      return BiquadFilterType::PEAKING;
    if (type == "notch")
      return BiquadFilterType::NOTCH;
    if (type == "allpass")
      return BiquadFilterType::ALLPASS;

    throw std::invalid_argument("Invalid filter type: " + type);
  }

  static std::string filterTypeToString(BiquadFilterType type) {
    switch (type) {
      case BiquadFilterType::LOWPASS:
        return "lowpass";
      case BiquadFilterType::HIGHPASS:
        return "highpass";
      case BiquadFilterType::BANDPASS:
        return "bandpass";
      case BiquadFilterType::LOWSHELF:
        return "lowshelf";
      case BiquadFilterType::HIGHSHELF:
        return "highshelf";
      case BiquadFilterType::PEAKING:
        return "peaking";
      case BiquadFilterType::NOTCH:
        return "notch";
      case BiquadFilterType::ALLPASS:
        return "allpass";
      default:
        throw std::invalid_argument("Unknown filter type");
    }
  }
};
} // namespace audioapi
