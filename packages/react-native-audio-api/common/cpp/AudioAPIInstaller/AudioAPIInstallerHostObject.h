#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

#include "AudioAPIInstallerWrapper.h"
#include "AudioContextHostObject.h"

namespace audioapi {
using namespace facebook;

class AudioAPIInstallerWrapper;

class AudioAPIInstallerHostObject : public jsi::HostObject {
 private:
  std::shared_ptr<AudioAPIInstallerWrapper> wrapper_;

 public:
  explicit AudioAPIInstallerHostObject(
      const std::shared_ptr<AudioAPIInstallerWrapper> &wrapper)
      : wrapper_(wrapper) {}

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
};
} // namespace audioapi
