#pragma once

#include <jsi/jsi.h>
#include <memory>

#include "BaseAudioContextHostObject.h"
#include "AudioContextWrapper.h"

namespace audioapi {
using namespace facebook;

class AudioContextHostObject : public BaseAudioContextHostObject {
 public:
  explicit AudioContextHostObject(
      const std::shared_ptr<AudioContextWrapper> &wrapper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  static std::shared_ptr<AudioContextHostObject> createFromWrapper(
      const std::shared_ptr<AudioContextWrapper> &wrapper) {
    return std::make_shared<AudioContextHostObject>(wrapper);
  }

 private:
  std::shared_ptr<AudioContextWrapper> getAudioContextWrapperFromBaseAudioContextWrapper();
};
} // namespace audioapi
