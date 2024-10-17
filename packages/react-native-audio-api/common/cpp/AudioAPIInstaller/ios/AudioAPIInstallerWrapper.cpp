#ifndef ANDROID
#include "AudioAPIInstallerWrapper.h"

namespace audioapi {

std::shared_ptr<AudioContextWrapper>
AudioAPIInstallerWrapper::createAudioContext() const {
    auto audioContext = std::make_shared<AudioContext>();
  return std::make_shared<audioapi::AudioContextWrapper>(audioContext);
}
} // namespace audioapi
#endif
