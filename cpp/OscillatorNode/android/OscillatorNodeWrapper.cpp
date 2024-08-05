#ifdef ANDROID
#include "OscillatorNodeWrapper.h"

namespace audiocontext {

    OscillatorNodeWrapper::OscillatorNodeWrapper(const std::shared_ptr<OscillatorNode> &oscillator) : AudioNodeWrapper(
            oscillator) {
        auto frequencyParam = oscillator->getFrequencyParam();
        frequencyParam_ = std::make_shared<AudioParamWrapper>(frequencyParam);
        auto detuneParam = oscillator->getDetuneParam();
        detuneParam_ = std::make_shared<AudioParamWrapper>(detuneParam);
    }

    OscillatorNodeWrapper::~OscillatorNodeWrapper() {
        auto oscillatorNode_ = std::static_pointer_cast<OscillatorNode>(node_);
        oscillatorNode_->prepareForDeconstruction();
    }

    std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getFrequencyParam() {
        return frequencyParam_;
    }

    std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getDetuneParam() {
        return detuneParam_;
    }

    std::string OscillatorNodeWrapper::getType() {
        auto oscillatorNode_ = std::static_pointer_cast<OscillatorNode>(node_);
        return oscillatorNode_->getWaveType();
    }

    void OscillatorNodeWrapper::start(double time) {
        auto oscillatorNode_ = std::static_pointer_cast<OscillatorNode>(node_);
        oscillatorNode_->start(time);
    }

    void OscillatorNodeWrapper::stop(double time) {
        auto oscillatorNode_ = std::static_pointer_cast<OscillatorNode>(node_);
        oscillatorNode_->stop(time);
    }

    void OscillatorNodeWrapper::setType(const std::string& type) {
        auto oscillatorNode_ = std::static_pointer_cast<OscillatorNode>(node_);
        oscillatorNode_->setWaveType(type);
    }
}
#endif
