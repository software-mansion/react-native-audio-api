#pragma once

#include <memory>
#include <string>
#include <utility>

#include "AudioContextWrapper.h"

#ifdef ANDROID
#include "AudioAPI.h"
#endif

namespace audioapi {

#ifdef ANDROID
    class AudioAPI;
#endif

    class AudioAPIWrapper {
#ifdef ANDROID

    private:
        AudioAPI *audioAPI_;

    public:
        explicit AudioAPIWrapper(AudioAPI *audioAPI): audioAPI_(audioAPI) {}
#else

 public:
  AudioAPIWrapper() {}
#endif
    public:
        std::shared_ptr<AudioContextWrapper> createAudioContext();
    };
} // namespace audioapi
