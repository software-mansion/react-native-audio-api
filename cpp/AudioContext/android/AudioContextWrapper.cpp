#ifdef ANDROID
#include "AudioContextWrapper.h"

namespace audiocontext {

    AudioContextWrapper::AudioContextWrapper(const std::shared_ptr<AudioContext> &audiocontext) : audiocontext_(audiocontext) {
        auto destinationNode = audiocontext_->getDestination();
        destinationNode_ = std::make_shared<AudioDestinationNodeWrapper>(destinationNode);
    }

    std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() {
        auto oscillator = audiocontext_->createOscillator();
        return std::make_shared<OscillatorNodeWrapper>(oscillator);
    }

    std::shared_ptr<AudioDestinationNodeWrapper> AudioContextWrapper::getDestination() {
        auto destination = audiocontext_->getDestination();
        return std::make_shared<AudioDestinationNodeWrapper>(destination);
    }

    std::shared_ptr<GainNodeWrapper> AudioContextWrapper::createGain() {
        auto gain = audiocontext_->createGain();
        return std::make_shared<GainNodeWrapper>(gain);
    }

    std::shared_ptr<StereoPannerNodeWrapper> AudioContextWrapper::createStereoPanner() {
        auto panner = audiocontext_->createStereoPanner();
        return std::make_shared<StereoPannerNodeWrapper>(panner);
    }
} // namespace audiocontext
#endif