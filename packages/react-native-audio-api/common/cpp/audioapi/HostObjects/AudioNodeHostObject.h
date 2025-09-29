#pragma once

#include <audioapi/jsi/JsiHostObject.h>

#include <jsi/jsi.h>
#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioNode;

class AudioNodeHostObject : public JsiHostObject {
 public:
  explicit AudioNodeHostObject(const std::shared_ptr<AudioNode> &node);

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
