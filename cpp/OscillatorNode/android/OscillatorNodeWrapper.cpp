#ifdef ANDROID
#include "OscillatorNodeWrapper.h"

namespace audiocontext {

    double OscillatorNodeWrapper::getFrequency() {
        return oscillator_->getFrequency();
    }

    double OscillatorNodeWrapper::getDetune() {
        return oscillator_->getDetune();
    }

    std::string OscillatorNodeWrapper::getType() {
        return oscillator_->getWaveType();
    }

    void OscillatorNodeWrapper::start(double time) {
        oscillator_->start(time);
    }

    void OscillatorNodeWrapper::stop(double time) {
        oscillator_->stop(time);
    }

    void OscillatorNodeWrapper::setFrequency(double frequency) {
        oscillator_->setFrequency(frequency);
    }

    void OscillatorNodeWrapper::setDetune(double detune) {
        oscillator_->setDetune(detune);
    }

    void OscillatorNodeWrapper::setType(const std::string& type) {
        oscillator_->setWaveType(type);
    }
}
#endif