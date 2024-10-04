#ifdef ANDROID
#include "AudioAPIInstallerWrapper.h"

namespace audioapi {

AudioAPIInstallerWrapper::AudioAPIInstallerWrapper(
    AudioAPIInstaller *audioAPIInstaller)
    : audioAPIInstaller_(audioAPIInstaller) {}

std::shared_ptr<AudioContextWrapper>
AudioAPIInstallerWrapper::createAudioContext() const {
  auto audioContext = audioAPIInstaller_->createAudioContext();
  return std::make_shared<AudioContextWrapper>(audioContext);
}
} // namespace audioapi
#endif
