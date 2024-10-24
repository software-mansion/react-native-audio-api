#include "BaseAudioContextWrapper.h"

namespace audioapi {

    BaseAudioContextWrapper::BaseAudioContextWrapper(
            const std::shared_ptr<BaseAudioContext> &context)
            : context_(context) {
        auto destination = context_->getDestination();
        destination_ = std::make_shared<AudioDestinationNodeWrapper>(destination);
    }

    std::shared_ptr<AudioDestinationNodeWrapper>
    BaseAudioContextWrapper::getDestination() const {
        return destination_;
    }

    std::shared_ptr<OscillatorNodeWrapper> BaseAudioContextWrapper::createOscillator()
    const {
        auto oscillator = context_->createOscillator();
        return std::make_shared<OscillatorNodeWrapper>(oscillator);
    }

    std::shared_ptr<GainNodeWrapper> BaseAudioContextWrapper::createGain() const {
        auto gain = context_->createGain();
        return std::make_shared<GainNodeWrapper>(gain);
    }

    std::shared_ptr<StereoPannerNodeWrapper>
    BaseAudioContextWrapper::createStereoPanner() const {
        auto panner = context_->createStereoPanner();
        return std::make_shared<StereoPannerNodeWrapper>(panner);
    }

    std::shared_ptr<BiquadFilterNodeWrapper>
    BaseAudioContextWrapper::createBiquadFilter() const {
        auto filter = context_->createBiquadFilter();
        return std::make_shared<BiquadFilterNodeWrapper>(filter);
    }

    std::shared_ptr<AudioBufferSourceNodeWrapper>
    BaseAudioContextWrapper::createBufferSource() const {
        auto bufferSource = context_->createBufferSource();
        return std::make_shared<AudioBufferSourceNodeWrapper>(bufferSource);
    }

    std::shared_ptr<AudioBufferWrapper> BaseAudioContextWrapper::createBuffer(
            int numberOfChannels,
            int length,
            int sampleRate) const {
        auto buffer =
                context_->createBuffer(numberOfChannels, length, sampleRate);
        return std::make_shared<AudioBufferWrapper>(buffer);
    }

    std::string BaseAudioContextWrapper::getState() const {
        return context_->getState();
    }

    int BaseAudioContextWrapper::getSampleRate() const {
        return context_->getSampleRate();
    }

    double BaseAudioContextWrapper::getCurrentTime() const {
        return context_->getCurrentTime();
    }
} // namespace audioapi
