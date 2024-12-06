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
  auto audioAPIInstallerWrapper =
      std::make_shared<AudioAPIInstallerWrapper>(this);
  AudioAPIInstallerHostObject::createAndInstallFromWrapper(
      audioAPIInstallerWrapper, rnRuntime_, jsCallInvoker_);
}

std::shared_ptr<AudioContext> AudioAPIInstaller::createAudioContext() {
  return std::make_shared<AudioContext>();
}

} // namespace audioapi
