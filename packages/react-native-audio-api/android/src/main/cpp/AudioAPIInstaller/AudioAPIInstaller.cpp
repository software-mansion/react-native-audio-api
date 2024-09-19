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
  static const auto method =
      javaClassLocal()->getMethod<AudioContext()>("createAudioContext");
  auto audioContext = method(javaPart_.get());

  return audioContext->cthis();
}

} // namespace audioapi
