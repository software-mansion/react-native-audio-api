#ifdef ANDROID
#include "OscillatorNodeWrapper.h"

namespace audiocontext {

    OscillatorNodeWrapper::OscillatorNodeWrapper(const std::shared_ptr<OscillatorNode> &oscillator) : AudioNodeWrapper(
            oscillator), oscillatorNode_(oscillator) {
        auto frequencyParam = oscillatorNode_->getFrequencyParam();
        frequencyParam_ = std::make_shared<AudioParamWrapper>(frequencyParam);
        auto detuneParam = oscillatorNode_->getDetuneParam();
        detuneParam_ = std::make_shared<AudioParamWrapper>(detuneParam);
    }

    std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getFrequencyParam() {
        return frequencyParam_;
    }

    std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getDetuneParam() {
        return detuneParam_;
    }

    std::string OscillatorNodeWrapper::getType() {
        return oscillatorNode_->getWaveType();
    }

    void OscillatorNodeWrapper::start(double time) {
        oscillatorNode_->start(time);
    }

    void OscillatorNodeWrapper::stop(double time) {
        oscillatorNode_->stop(time);
    }

    void OscillatorNodeWrapper::setType(const std::string& type) {
        oscillatorNode_->setWaveType(type);
    }
}
#endif