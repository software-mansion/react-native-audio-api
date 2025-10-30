#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class ConvolverNode;

class ConvolverNodeHostObject : public AudioNodeHostObject {
 public:
  explicit ConvolverNodeHostObject(const std::shared_ptr<ConvolverNode> &node);
  JSI_PROPERTY_GETTER_DECL(normalize);
  JSI_PROPERTY_GETTER_DECL(buffer);
  JSI_PROPERTY_SETTER_DECL(normalize);
  JSI_PROPERTY_SETTER_DECL(buffer);
};
} // namespace audioapi
