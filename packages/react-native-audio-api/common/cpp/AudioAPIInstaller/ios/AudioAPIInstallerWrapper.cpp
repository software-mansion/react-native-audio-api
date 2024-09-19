#ifndef ANDROID
#include "AudioAPIInstallerWrapper.h"

namespace audioapi {
std::shared_ptr<AudioContextWrapper>
AudioAPIInstallerWrapper::createAudioContext() {
  return std::make_shared<audioapi::AudioContextWrapper>();
}
} // namespace audioapi
#endif
