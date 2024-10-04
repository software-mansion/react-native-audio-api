#include "AudioAPIInstaller.h"

namespace audioapi {

using namespace facebook::jni;

AudioAPIInstaller::AudioAPIInstaller(
    jni::alias_ref<AudioAPIInstaller::jhybridobject> &jThis)
    : javaPart_(make_global(jThis)) {}

void AudioAPIInstaller::install(jlong jsContext) {
  auto audioAPIInstallerWrapper =
      std::make_shared<AudioAPIInstallerWrapper>(this);
  AudioAPIInstallerHostObject::createAndInstallFromWrapper(
      audioAPIInstallerWrapper, jsContext);
}

std::shared_ptr<AudioContext> AudioAPIInstaller::createAudioContext() {
  return std::make_shared<AudioContext>();
}

} // namespace audioapi
