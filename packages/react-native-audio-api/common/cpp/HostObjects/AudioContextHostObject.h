#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include "AudioContext.h"
#include "BaseAudioContextHostObject.h"
#include "JsiPromise.h"

namespace audioapi {
using namespace facebook;

class AudioContextHostObject : public BaseAudioContextHostObject {
 public:
  explicit AudioContextHostObject(
      const std::shared_ptr<AudioContext> &audioContext,
      const std::shared_ptr<JsiPromise::PromiseVendor>& promiseVendor);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

 private:
  std::shared_ptr<AudioContext>
  getAudioContextFromBaseAudioContext();
};
} // namespace audioapi
