#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct StereoPannerOptions;
class BaseAudioContext;
class AudioParamHostObject;
class StereoPannerNode;

class StereoPannerNodeHostObject : public AudioNodeHostObject {
 public:
  explicit StereoPannerNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const StereoPannerOptions &options);

  JSI_PROPERTY_GETTER_DECL(pan);

  [[nodiscard]] size_t getMemoryPressure() const override {
    return AudioNodeHostObject::getMemoryPressure() + kAudioParamBytes;
  }

 private:
  StereoPannerNode *stereoPannerNode_ = nullptr;

  std::shared_ptr<AudioParamHostObject> panParam_;
};
} // namespace audioapi
