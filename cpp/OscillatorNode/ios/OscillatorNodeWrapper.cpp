#ifndef ANDROID
#include "OscillatorNodeWrapper.h"

namespace audiocontext {

    double OscillatorNodeWrapper::getFrequency() {
        return oscillator_->getFrequency();
    }

    double OscillatorNodeWrapper::getDetune() {
        return oscillator_->getDetune();
    }

    std::string OscillatorNodeWrapper::getType() {
        return oscillator_->getType();
    }

    void OscillatorNodeWrapper::start(double time) {
        oscillator_->start();
    }

    void OscillatorNodeWrapper::stop(double time) {
        oscillator_->stop();
    }

    void OscillatorNodeWrapper::setFrequency(double frequency) {
        oscillator_->changeFrequency(frequency);
    }

    void OscillatorNodeWrapper::setDetune(double detune) {
        oscillator_->changeDetune(detune);
    }

    void OscillatorNodeWrapper::setType(const std::string& type) {
        oscillator_->setType(type);
    }
}
#endif
