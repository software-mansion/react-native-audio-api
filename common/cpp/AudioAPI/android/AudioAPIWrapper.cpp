#ifdef ANDROID
#include "AudioAPIWrapper.h"

namespace audioapi {
    std::shared_ptr<AudioContextWrapper> AudioAPIWrapper::createAudioContext() {
        auto audioContext = audioAPI_->createAudioContext();
        return std::make_shared<AudioContextWrapper>(audioContext);
    }
} // namespace audioapi
#endif
