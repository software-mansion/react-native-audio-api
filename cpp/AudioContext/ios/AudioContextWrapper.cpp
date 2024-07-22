#ifndef ANDROID
#include "AudioContextWrapper.h"

namespace audiocontext {

    std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() {
        return std::make_shared<OscillatorNodeWrapper>();
    }

    std::shared_ptr<AudioDestinationNodeWrapper> AudioContextWrapper::getDestination() {
        return std::make_shared<AudioDestinationNodeWrapper>();
    }
} // namespace audiocontext
#endif
