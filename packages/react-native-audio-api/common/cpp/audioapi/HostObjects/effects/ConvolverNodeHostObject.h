#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct ConvolverOptions;
class BaseAudioContext;

class ConvolverNodeHostObject : public AudioNodeHostObject {
 public:
  explicit ConvolverNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const ConvolverOptions &options);
  JSI_PROPERTY_GETTER_DECL(normalize);
  JSI_PROPERTY_GETTER_DECL(buffer);
  JSI_PROPERTY_SETTER_DECL(normalize);
  JSI_HOST_FUNCTION_DECL(setBuffer);
};
} // namespace audioapi
