#ifdef ANDROID
#include "AudioContextWrapper.h"

namespace audiocontext {

    std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() {
        auto oscillator = audiocontext_->createOscillator();
        return std::make_shared<OscillatorNodeWrapper>(oscillator);
    }

    std::shared_ptr<AudioDestinationNodeWrapper> AudioContextWrapper::getDestination() {
        auto destination = audiocontext_->getDestination();
        return std::make_shared<AudioDestinationNodeWrapper>(destination);
    }
} // namespace audiocontext
#endif