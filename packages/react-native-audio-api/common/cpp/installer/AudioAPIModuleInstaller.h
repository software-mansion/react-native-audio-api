#pragma once

#include <JsiPromise.h>

namespace audioapi {

using namespace facebook;

class AudioAPIModuleInstaller {
public:
    explicit AudioAPIModuleInstaller(jsi::Runtime *jsiRuntime, const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker);
    void injectJSIBindings(jsi::Runtime *jsiRuntime);

private:
  std::shared_ptr<PromiseVendor> promiseVendor_;

  jsi::Function getCreateAudioContextFunction(jsi::Runtime *jsiRuntime);
};

} // namespace audioapi
