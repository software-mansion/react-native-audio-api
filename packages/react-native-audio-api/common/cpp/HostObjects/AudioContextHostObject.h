#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include"JsiPromise.h"
#include "AudioContextWrapper.h"
#include "BaseAudioContextHostObject.h"

namespace audioapi {
using namespace facebook;

class AudioContextHostObject : public BaseAudioContextHostObject {
 public:
  explicit AudioContextHostObject(
      const std::shared_ptr<AudioContextWrapper> &wrapper, std::shared_ptr<JsiPromise::PromiseVendor> promiseVendor);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  static std::shared_ptr<AudioContextHostObject> createFromWrapper(
      const std::shared_ptr<AudioContextWrapper> &wrapper, std::shared_ptr<JsiPromise::PromiseVendor> promiseVendor) {
    return std::make_shared<AudioContextHostObject>(wrapper, promiseVendor);
  }

 private:
  std::shared_ptr<AudioContextWrapper>
  getAudioContextWrapperFromBaseAudioContextWrapper();
};
} // namespace audioapi
