#ifndef ANDROID
#include "OscillatorNodeWrapper.h"

namespace audiocontext
{
    OscillatorNodeWrapper::OscillatorNodeWrapper(std::shared_ptr<IOSAudioContext> context) : AudioNodeWrapper() {
        node_ = oscillator_ = std::make_shared<IOSOscillator>(context);
        frequencyParam_ = std::make_shared<AudioParamWrapper>(oscillator_->getFrequencyParam());
        detuneParam_ = std::make_shared<AudioParamWrapper>(oscillator_->getDetuneParam());
    }

    std::string OscillatorNodeWrapper::getType() {
        return oscillator_->getType();
    }

    void OscillatorNodeWrapper::start(double time) {
        oscillator_->start(time);
    }

    void OscillatorNodeWrapper::stop(double time) {
        oscillator_->stop(time);
    }

    void OscillatorNodeWrapper::setType(const std::string& type) {
        oscillator_->setType(type);
    }

    std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getFrequencyParam() const {
        return frequencyParam_;
    }

    std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getDetuneParam() const {
        return detuneParam_;
    }
} // namespace audiocontext
#endif
