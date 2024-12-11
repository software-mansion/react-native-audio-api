#pragma once

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

#include "AudioContextHostObject.h"
#include "JsiPromise.h"

namespace audioapi {
using namespace facebook;

class AudioAPIInstallerHostObject : public jsi::HostObject, public std::enable_shared_from_this<AudioAPIInstallerHostObject> {

public:
  explicit AudioAPIInstallerHostObject(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &jsInvoker);

    jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

    void set(
            jsi::Runtime &runtime,
            const jsi::PropNameID &name,
            const jsi::Value &value) override;

    std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

    void install() {
        auto object = jsi::Object::createFromHostObject(*rnRuntime_, shared_from_this());
        rnRuntime_->global().setProperty(
                *rnRuntime_, "__AudioAPIInstaller", std::move(object));
    }

 private:
  std::shared_ptr<JsiPromise::PromiseVendor> promiseVendor_;
    jsi::Runtime *rnRuntime_;
};
} // namespace audioapi
