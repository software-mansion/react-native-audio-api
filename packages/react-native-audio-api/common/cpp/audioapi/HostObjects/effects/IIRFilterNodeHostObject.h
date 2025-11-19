#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class IIRFilterNode;

class IIRFilterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit IIRFilterNodeHostObject(const std::shared_ptr<IIRFilterNode> &node);

  JSI_HOST_FUNCTION_DECL(getFrequencyResponse);
};
} // namespace audioapi
