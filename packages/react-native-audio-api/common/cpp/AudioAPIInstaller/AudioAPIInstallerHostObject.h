#pragma once

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

#include <JsiHostObject.h>
#include "AudioContextHostObject.h"
#include "JsiPromise.h"

namespace audioapi {
using namespace facebook;

class AudioAPIInstallerHostObject
    : public JsiHostObject,
      public std::enable_shared_from_this<AudioAPIInstallerHostObject> {
 public:
  explicit AudioAPIInstallerHostObject(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &jsInvoker)
      : rnRuntime_(runtime) {
    promiseVendor_ =
        std::make_shared<JsiPromise::PromiseVendor>(runtime, jsInvoker);

    addFunctions(
        JSI_EXPORT_FUNCTION(AudioAPIInstallerHostObject, createAudioContext));
  }

  void install() {
    auto object =
        jsi::Object::createFromHostObject(*rnRuntime_, shared_from_this());
    rnRuntime_->global().setProperty(
        *rnRuntime_, "__AudioAPIInstaller", std::move(object));
  }

  JSI_HOST_FUNCTION(createAudioContext) {
    auto audioContext = std::make_shared<AudioContext>();
    auto audioContextHostObject =
        std::make_shared<AudioContextHostObject>(audioContext, promiseVendor_);
    return jsi::Object::createFromHostObject(runtime, audioContextHostObject);
  }

 private:
  std::shared_ptr<JsiPromise::PromiseVendor> promiseVendor_;
  jsi::Runtime *rnRuntime_;
};
} // namespace audioapi
