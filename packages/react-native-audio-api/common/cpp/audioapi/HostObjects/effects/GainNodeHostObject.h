#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct GainOptions;
class BaseAudioContext;
class AudioParamHostObject;
class GainNode;

class GainNodeHostObject : public AudioNodeHostObject {
 public:
  explicit GainNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const GainOptions &options);

  JSI_PROPERTY_GETTER_DECL(gain);

  [[nodiscard]] size_t getMemoryPressure() const override {
    return AudioNodeHostObject::getMemoryPressure() + kAudioParamBytes;
  }

 private:
  GainNode *gainNode_ = nullptr;

  std::shared_ptr<AudioParamHostObject> gainParam_;
};
} // namespace audioapi
