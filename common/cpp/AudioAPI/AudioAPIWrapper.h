#pragma once

#include <memory>
#include <string>
#include <utility>

#include "AudioContextWrapper.h"

#ifdef ANDROID
#include "AudioAPI.h"
#else
#include "IOSAudioAPI.h"
#endif

namespace audioapi {
    using namespace facebook;

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

        private:
  std::shared_ptr<IOSAudioAPI> audioAPI_;

 public:
  AudioAPIWrapper();
#endif
    public:
        std::shared_ptr<AudioContextWrapper> createAudioContext();
    };
} // namespace audioapi
