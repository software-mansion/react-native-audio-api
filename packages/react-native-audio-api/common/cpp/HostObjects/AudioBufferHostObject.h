#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include "AudioBufferWrapper.h"

namespace audioapi {
using namespace facebook;

class AudioBufferHostObject : public jsi::HostObject {
 public:
  std::shared_ptr<AudioBufferWrapper> wrapper_;

  explicit AudioBufferHostObject(
      const std::shared_ptr<AudioBufferWrapper> &wrapper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  static std::shared_ptr<AudioBufferHostObject> createFromWrapper(
      const std::shared_ptr<AudioBufferWrapper> &wrapper) {
    return std::make_shared<AudioBufferHostObject>(wrapper);
  }
};
} // namespace audioapi
