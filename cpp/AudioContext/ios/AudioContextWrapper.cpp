#ifndef ANDROID
#include "AudioContextWrapper.h"

namespace audiocontext {

    std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() {
        return std::make_shared<OscillatorNodeWrapper>(audiocontext_);
    }

    std::shared_ptr<AudioDestinationNodeWrapper> AudioContextWrapper::getDestination() {
        throw std::runtime_error("[AudioContextHostObject::getDestination] Not yet implemented!");
    }

    std::shared_ptr<GainNodeWrapper> AudioContextWrapper::createGain() {
        return std::make_shared<GainNodeWrapper>(audiocontext_);
    }

    std::shared_ptr<StereoPannerNodeWrapper> AudioContextWrapper::createStereoPanner() {
        throw std::runtime_error("[AudioContextHostObject::createStereoPanner] Not yet implemented!");
    }
} // namespace audiocontext
#endif
