#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/analysis/AnalyserNode.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class AnalyserNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AnalyserNodeHostObject(const std::shared_ptr<AnalyserNode> &node);

  JSI_PROPERTY_GETTER_DECL(fftSize);
  JSI_PROPERTY_GETTER_DECL(frequencyBinCount);
  JSI_PROPERTY_GETTER_DECL(minDecibels);
  JSI_PROPERTY_GETTER_DECL(maxDecibels);
  JSI_PROPERTY_GETTER_DECL(smoothingTimeConstant);
  JSI_PROPERTY_GETTER_DECL(window);

  JSI_PROPERTY_SETTER_DECL(fftSize);
  JSI_PROPERTY_SETTER_DECL(minDecibels);
  JSI_PROPERTY_SETTER_DECL(maxDecibels);
  JSI_PROPERTY_SETTER_DECL(smoothingTimeConstant);
  JSI_PROPERTY_SETTER_DECL(window);

  JSI_HOST_FUNCTION_DECL(getFloatFrequencyData);
  JSI_HOST_FUNCTION_DECL(getByteFrequencyData);
  JSI_HOST_FUNCTION_DECL(getFloatTimeDomainData);
  JSI_HOST_FUNCTION_DECL(getByteTimeDomainData);

  static std::string windowTypeToString(const AnalyserNode::WindowType type) {
    switch (type) {
      case AnalyserNode::WindowType::BLACKMAN:
        return "blackman";
      case AnalyserNode::WindowType::HANN:
        return "hann";
      default:
        throw std::invalid_argument("Unknown window type");
    }
  }

  static AnalyserNode::WindowType windowTypeFromString(const std::string &type) {
    if (type == "blackman") {
      return AnalyserNode::WindowType::BLACKMAN;
    }
    if (type == "hann") {
      return AnalyserNode::WindowType::HANN;
    }

    throw std::invalid_argument("Unknown window type");
  }
};

} // namespace audioapi
