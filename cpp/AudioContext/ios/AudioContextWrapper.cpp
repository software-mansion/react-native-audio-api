#ifndef ANDROID
#include "AudioContextWrapper.h"

namespace audiocontext {

    std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() {
        return std::make_shared<OscillatorNodeWrapper>();
    }

    std::shared_ptr<AudioDestinationNodeWrapper> AudioContextWrapper::getDestination() {
        throw std::runtime_error("[AudioContextHostObject::getDestination] Not yet implemented!");
    }

    std::shared_ptr<GainNodeWrapper> AudioContextWrapper::createGainNode() {
        return std::make_shared<GainNodeWrapper>();
    }
} // namespace audiocontext
#endif
