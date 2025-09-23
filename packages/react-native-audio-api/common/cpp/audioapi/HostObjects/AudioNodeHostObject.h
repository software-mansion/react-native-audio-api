#pragma once

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/jsi/JsiHostObject.h>

#include <jsi/jsi.h>
#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioNodeHostObject : public JsiHostObject {
 public:
  explicit AudioNodeHostObject(const std::shared_ptr<AudioNode> &node)
      : node_(node) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, numberOfInputs),
        JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, numberOfOutputs),
        JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, channelCount),
        JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, channelCountMode),
        JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, channelInterpretation));

    addFunctions(
        JSI_EXPORT_FUNCTION(AudioNodeHostObject, connect),
        JSI_EXPORT_FUNCTION(AudioNodeHostObject, disconnect));
  }

  JSI_PROPERTY_GETTER_DECL(numberOfInputs);
  JSI_PROPERTY_GETTER_DECL(numberOfOutputs);
  JSI_PROPERTY_GETTER_DECL(channelCount);
  JSI_PROPERTY_GETTER_DECL(channelCountMode);
  JSI_PROPERTY_GETTER_DECL(channelInterpretation);

  JSI_HOST_FUNCTION_DECL(connect);
  JSI_HOST_FUNCTION_DECL(disconnect);

 protected:
  std::shared_ptr<AudioNode> node_;
};
} // namespace audioapi
