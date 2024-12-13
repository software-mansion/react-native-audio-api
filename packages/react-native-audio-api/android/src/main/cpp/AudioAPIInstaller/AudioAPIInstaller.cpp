#include "AudioAPIInstaller.h"

namespace audioapi {

using namespace facebook::jni;

AudioAPIInstaller::AudioAPIInstaller(
    jni::alias_ref<AudioAPIInstaller::jhybridobject> &jThis,
    jsi::Runtime *rnRuntime,
    const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker)
    : javaPart_(make_global(jThis)),
      rnRuntime_(rnRuntime),
      jsCallInvoker_(jsCallInvoker) {}

void AudioAPIInstaller::install() {
  auto hostObject =
      std::make_shared<AudioAPIInstallerHostObject>(rnRuntime_, jsCallInvoker_);
  hostObject->install();
}
} // namespace audioapi
