#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include "AudioBuffer.h"

namespace audioapi {
using namespace facebook;

class AudioBufferHostObject : public jsi::HostObject {
 public:
  std::shared_ptr<AudioBuffer> audioBuffer_;

  explicit AudioBufferHostObject(
      const std::shared_ptr<AudioBuffer> &audioBuffer);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;
};
} // namespace audioapi
