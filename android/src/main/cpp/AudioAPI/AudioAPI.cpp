#include "AudioAPI.h"

namespace audioapi {

using namespace facebook::jni;

AudioAPI::AudioAPI(jni::alias_ref<AudioAPI::jhybridobject> &jThis)
    : javaPart_(make_global(jThis)) {}

void AudioAPI::install(jlong jsContext) {
  auto audioAPIWrapper = std::make_shared<AudioAPIWrapper>(this);
  AudioAPIHostObject::createAndInstallFromWrapper(audioAPIWrapper, jsContext);
}

AudioContext *AudioAPI::createAudioContext() {
  static const auto method =
      javaClassLocal()->getMethod<AudioContext()>("createAudioContext");
  auto audioContext = method(javaPart_.get());

  return audioContext->cthis();
}

} // namespace audioapi
