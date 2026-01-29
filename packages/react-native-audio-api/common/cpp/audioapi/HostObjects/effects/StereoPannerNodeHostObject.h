#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

struct StereoPannerOptions;
class BaseAudioContext;

class StereoPannerNodeHostObject : public AudioNodeHostObject {
 public:
  explicit StereoPannerNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const StereoPannerOptions &options);

  JSI_PROPERTY_GETTER_DECL(pan);
};
} // namespace audioapi
