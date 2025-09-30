#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class BiquadFilterNode;

class BiquadFilterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit BiquadFilterNodeHostObject(
      const std::shared_ptr<BiquadFilterNode> &node);

  JSI_PROPERTY_GETTER_DECL(frequency);
  JSI_PROPERTY_GETTER_DECL(detune);
  JSI_PROPERTY_GETTER_DECL(Q);
  JSI_PROPERTY_GETTER_DECL(gain);
  JSI_PROPERTY_GETTER_DECL(type);

  JSI_PROPERTY_SETTER_DECL(type);

  JSI_HOST_FUNCTION_DECL(getFrequencyResponse);
};
} // namespace audioapi
