#ifdef ANDROID
#include "AudioAPIInstallerWrapper.h"

namespace audioapi {
std::shared_ptr<AudioContextWrapper>
AudioAPIInstallerWrapper::createAudioContext() {
  auto audioContext = audioAPIInstaller_->createAudioContext();
  return std::make_shared<AudioContextWrapper>(audioContext);
}
} // namespace audioapi
#endif
