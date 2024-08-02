#ifndef ANDROID
#include "AudioContextWrapper.h"

namespace audiocontext
{
    std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() {
        return std::make_shared<OscillatorNodeWrapper>(audiocontext_);
    }

    std::shared_ptr<AudioDestinationNodeWrapper> AudioContextWrapper::getDestination() {
        // TODO: Add AudioDestinationNode implementation
        return std::make_shared<AudioDestinationNodeWrapper>();
    }

    std::shared_ptr<GainNodeWrapper> AudioContextWrapper::createGain() {
        return std::make_shared<GainNodeWrapper>(audiocontext_);
    }

    std::shared_ptr<StereoPannerNodeWrapper> AudioContextWrapper::createStereoPanner() {
        return std::make_shared<StereoPannerNodeWrapper>(audiocontext_);
    }

    std::string AudioContextWrapper::getState() {
        throw std::runtime_error("[AudioContextHostObject] getState not implemented yet!");
    }
    int AudioContextWrapper::getSampleRate() {
        throw std::runtime_error("[AudioContextHostObject] getSampleRate not implemented yet!");
    }
    double AudioContextWrapper::getCurrentTime() {
        throw std::runtime_error("[AudioContextHostObject] getCurrentTime not implemented yet!");
    }
} // namespace audiocontext
#endif
