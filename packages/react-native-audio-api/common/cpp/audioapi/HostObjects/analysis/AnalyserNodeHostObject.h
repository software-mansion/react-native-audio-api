#pragma once

#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/HostObjects/core/AudioNodeHostObject.h>

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

    addSetters(
          JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, fftSize),
          JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, minDecibels),
          JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, maxDecibels),
          JSI_EXPORT_PROPERTY_SETTER(
                  AnalyserNodeHostObject, smoothingTimeConstant),
          JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, window));

    addFunctions(
        JSI_EXPORT_FUNCTION(
            AnalyserNodeHostObject, getFloatFrequencyData),
        JSI_EXPORT_FUNCTION(
            AnalyserNodeHostObject, getByteFrequencyData),
        JSI_EXPORT_FUNCTION(
            AnalyserNodeHostObject, getFloatTimeDomainData),
        JSI_EXPORT_FUNCTION(
            AnalyserNodeHostObject, getByteTimeDomainData));
  }

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
