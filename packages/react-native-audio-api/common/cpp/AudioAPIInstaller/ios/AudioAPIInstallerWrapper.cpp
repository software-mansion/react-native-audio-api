#ifndef ANDROID
#include "AudioAPIInstallerWrapper.h"

namespace audioapi {

std::shared_ptr<AudioContextWrapper>
AudioAPIInstallerWrapper::createAudioContext() const {
  return std::make_shared<audioapi::AudioContextWrapper>();
}
} // namespace audioapi
#endif
