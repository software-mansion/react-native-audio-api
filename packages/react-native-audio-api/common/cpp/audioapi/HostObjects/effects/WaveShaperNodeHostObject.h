#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/types/OverSampleType.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class WaveShaperNode;

class WaveShaperNodeHostObject : public AudioNodeHostObject {
 public:
  explicit WaveShaperNodeHostObject(const std::shared_ptr<WaveShaperNode> &node);

  JSI_PROPERTY_GETTER_DECL(oversample);
  JSI_PROPERTY_GETTER_DECL(curve);

  JSI_PROPERTY_SETTER_DECL(oversample);
  JSI_HOST_FUNCTION_DECL(setCurve);

  static std::string overSampleTypeToString(const OverSampleType type) {
    switch (type) {
      case OverSampleType::OVERSAMPLE_2X:
        return "2x";
      case OverSampleType::OVERSAMPLE_4X:
        return "4x";
      default:
        return "none";
    }
  }

  static OverSampleType overSampleTypeFromString(const std::string &type) {
    if (type == "2x")
      return OverSampleType::OVERSAMPLE_2X;
    if (type == "4x")
      return OverSampleType::OVERSAMPLE_4X;

    return OverSampleType::OVERSAMPLE_NONE;
  }
};
} // namespace audioapi
