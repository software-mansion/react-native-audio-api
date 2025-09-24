#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class AnalyserNode;

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
};

} // namespace audioapi
