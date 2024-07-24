#pragma once

#include <memory>
#include <utility>
#include "OscillatorNodeWrapper.h"
#include "AudioDestinationNodeWrapper.h"
#include "GainNodeWrapper.h"

#ifdef ANDROID
#include "AudioContext.h"
#else
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
#ifdef ANDROID
    private:
        std::shared_ptr<AudioContext> audiocontext_;
#endif

    public:
#ifdef ANDROID
        explicit AudioContextWrapper(
                std::shared_ptr<AudioContext> audiocontext) : audiocontext_(audiocontext) {}
#else
        explicit AudioContextWrapper() {}
#endif
        std::shared_ptr<OscillatorNodeWrapper> createOscillator();
        std::shared_ptr<AudioDestinationNodeWrapper> getDestination();
        std::shared_ptr<GainNodeWrapper> createGainNode();
    };
} // namespace audiocontext
