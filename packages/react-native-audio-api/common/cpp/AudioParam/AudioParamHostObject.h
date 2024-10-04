#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include "AudioParamWrapper.h"

namespace audioapi {
using namespace facebook;

class AudioParamWrapper;

class AudioParamHostObject : public jsi::HostObject {
 public:
  explicit AudioParamHostObject(
      const std::shared_ptr<AudioParamWrapper> &wrapper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  static std::shared_ptr<AudioParamHostObject> createFromWrapper(
      const std::shared_ptr<AudioParamWrapper> &wrapper) {
    return std::make_shared<AudioParamHostObject>(wrapper);
  }

 private:
  std::shared_ptr<AudioParamWrapper> wrapper_;
};
} // namespace audioapi
