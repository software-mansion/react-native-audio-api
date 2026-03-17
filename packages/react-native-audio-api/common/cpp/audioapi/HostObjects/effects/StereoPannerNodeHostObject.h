#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct StereoPannerOptions;
class BaseAudioContext;
class AudioParamHostObject;

class StereoPannerNodeHostObject : public AudioNodeHostObject {
 public:
  explicit StereoPannerNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const StereoPannerOptions &options);

  JSI_PROPERTY_GETTER_DECL(pan);

 private:
  std::shared_ptr<AudioParamHostObject> panParam_;
};
} // namespace audioapi
