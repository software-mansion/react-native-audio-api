#ifndef ANDROID
#include "AudioAPIWrapper.h"

namespace audioapi {
std::shared_ptr<AudioContextWrapper> AudioAPIWrapper::createAudioContext() {
    return std::make_shared<audioapi::AudioContextWrapper>();
    }
} // namespace audioapi
#endif
