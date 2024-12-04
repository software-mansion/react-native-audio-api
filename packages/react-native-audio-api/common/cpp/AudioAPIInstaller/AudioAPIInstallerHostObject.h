#pragma once

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>


#include "AudioAPIInstallerWrapper.h"
#include "AudioContextHostObject.h"
#include "JsiPromise.h"

namespace audioapi {
using namespace facebook;

class AudioAPIInstallerWrapper;

class AudioAPIInstallerHostObject : public jsi::HostObject {
 public:
  explicit AudioAPIInstallerHostObject(
      const std::shared_ptr<AudioAPIInstallerWrapper> &wrapper, jsi::Runtime* runtime, const std::shared_ptr<react::CallInvoker> &jsInvoker);

#ifdef ANDROID
  static void createAndInstallFromWrapper(
      const std::shared_ptr<AudioAPIInstallerWrapper> &wrapper,
      jlong jsContext) {
    auto runtime = reinterpret_cast<jsi::Runtime *>(jsContext);
    auto hostObject = std::make_shared<AudioAPIInstallerHostObject>(wrapper);
    auto object = jsi::Object::createFromHostObject(*runtime, hostObject);
    runtime->global().setProperty(
        *runtime, "__AudioAPIInstaller", std::move(object));
  }
#endif

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

 private:
  std::shared_ptr<AudioAPIInstallerWrapper> wrapper_;
  std::shared_ptr<JsiPromise::PromiseVendor> promiseVendor_;
};
} // namespace audioapi
