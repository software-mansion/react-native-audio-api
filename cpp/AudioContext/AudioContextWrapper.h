#pragma once

#include <memory>
#include <utility>
#include "OscillatorNodeWrapper.h"
#include "AudioDestinationNodeWrapper.h"
#include "GainNodeWrapper.h"
#include "StereoPannerNodeWrapper.h"

#ifdef ANDROID
#include "AudioContext.h"
#else
#include "IOSAudioContext.h"
#include "OscillatorNodeHostObject.h"
#endif

namespace audiocontext {
    using namespace facebook;

    class OscillatorNodeWrapper;
    class AudioDestinationNodeWrapper;

#ifdef ANDROID
    class AudioContext;
#endif

    class AudioContextWrapper {
    private:
#ifdef ANDROID
        std::shared_ptr<AudioContext> audiocontext_;
#else
        std::shared_ptr<IOSAudioContext> audiocontext_;
#endif

    public:
#ifdef ANDROID
        explicit AudioContextWrapper(
                const std::shared_ptr<AudioContext> &audiocontext) : audiocontext_(audiocontext) {}
#else
        explicit AudioContextWrapper(): audiocontext_(std::make_shared<IOSAudioContext>()) {}
#endif
        std::shared_ptr<OscillatorNodeWrapper> createOscillator();
        std::shared_ptr<AudioDestinationNodeWrapper> getDestination();
        std::shared_ptr<GainNodeWrapper> createGain();
        std::shared_ptr<StereoPannerNodeWrapper> createStereoPanner();
    };
} // namespace audiocontext
