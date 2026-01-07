#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
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
};
} // namespace audioapi
