#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct GainOptions;
class BaseAudioContext;
class AudioParamHostObject;

class GainNodeHostObject : public AudioNodeHostObject {
 public:
  explicit GainNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const GainOptions &options);

  JSI_PROPERTY_GETTER_DECL(gain);

 private:
  std::shared_ptr<AudioParamHostObject> gainParam_;
};
} // namespace audioapi
