#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class ConstantSourceNode;

class ConstantSourceNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit ConstantSourceNodeHostObject(const std::shared_ptr<ConstantSourceNode> &node);

  JSI_PROPERTY_GETTER_DECL(offset);
};
} // namespace audioapi
