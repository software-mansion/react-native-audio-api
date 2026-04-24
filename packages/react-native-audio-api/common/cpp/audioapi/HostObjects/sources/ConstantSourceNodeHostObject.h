#pragma once

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct ConstantSourceOptions;
class BaseAudioContext;
class AudioParamHostObject;

class ConstantSourceNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit ConstantSourceNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const ConstantSourceOptions &options);

  JSI_PROPERTY_GETTER_DECL(offset);

  [[nodiscard]] size_t getMemoryPressure() const override {
    return AudioNodeHostObject::getMemoryPressure() + kAudioParamBytes;
  }

 private:
  std::shared_ptr<AudioParamHostObject> offsetParam_;
};
} // namespace audioapi
