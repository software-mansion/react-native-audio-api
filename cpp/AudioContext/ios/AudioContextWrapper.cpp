#ifndef ANDROID
#include "AudioContextWrapper.h"

namespace audiocontext
{
    std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() {
        return std::make_shared<OscillatorNodeWrapper>(audiocontext_);
    }

    std::shared_ptr<AudioDestinationNodeWrapper> AudioContextWrapper::getDestination() const {
        // TODO: Add AudioDestinationNode implementation
        return std::make_shared<AudioDestinationNodeWrapper>();
    }

    std::shared_ptr<GainNodeWrapper> AudioContextWrapper::createGain() {
        return std::make_shared<GainNodeWrapper>(audiocontext_);
    }

    std::shared_ptr<StereoPannerNodeWrapper> AudioContextWrapper::createStereoPanner() {
        return std::make_shared<StereoPannerNodeWrapper>(audiocontext_);
    }

    std::shared_ptr<BiquadFilterNodeWrapper> AudioContextWrapper::createBiquadFilter() {
        // TODO: Add BiquadFilterNode implementation
        return std::make_shared<BiquadFilterNodeWrapper>();
    }

    double AudioContextWrapper::getCurrentTime() {
        return audiocontext_->getCurrentTime();
    }

    std::string AudioContextWrapper::getState() {
        return audiocontext_->getState();
    }

    int AudioContextWrapper::getSampleRate() const {
        return audiocontext_->getSampleRate();
    }
} // namespace audiocontext
#endif
