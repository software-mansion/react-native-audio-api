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

AudioContext *AudioAPIInstaller::createAudioContext() {

  return new AudioContext();
}

} // namespace audioapi
