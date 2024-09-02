#ifdef ANDROID
#include "OscillatorNodeWrapper.h"

namespace audiocontext {

    OscillatorNode* OscillatorNodeWrapper::getOscillatorNodeFromAudioNode() {
        return dynamic_cast<OscillatorNode*>(node_);
    }

    OscillatorNodeWrapper::OscillatorNodeWrapper(OscillatorNode *oscillator) : AudioNodeWrapper(
            oscillator) {
        auto frequencyParam = oscillator->getFrequencyParam();
        frequencyParam_ = std::make_shared<AudioParamWrapper>(frequencyParam);
        auto detuneParam = oscillator->getDetuneParam();
        detuneParam_ = std::make_shared<AudioParamWrapper>(detuneParam);
    }

    OscillatorNodeWrapper::~OscillatorNodeWrapper() {
        auto oscillatorNode_ = getOscillatorNodeFromAudioNode();
        oscillatorNode_->prepareForDeconstruction();
    }

    std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getFrequencyParam() const {
        return frequencyParam_;
    }

    std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getDetuneParam() const {
        return detuneParam_;
    }

    std::string OscillatorNodeWrapper::getType() {
        auto oscillatorNode_ = getOscillatorNodeFromAudioNode();
        return oscillatorNode_->getWaveType();
    }

    void OscillatorNodeWrapper::start(double time) {
        auto oscillatorNode_ = getOscillatorNodeFromAudioNode();
        oscillatorNode_->start(time);
    }

    void OscillatorNodeWrapper::stop(double time) {
        auto oscillatorNode_ = getOscillatorNodeFromAudioNode();
        oscillatorNode_->stop(time);
    }

    void OscillatorNodeWrapper::setType(const std::string& type) {
        auto oscillatorNode_ = getOscillatorNodeFromAudioNode();
        oscillatorNode_->setWaveType(type);
    }
}
#endif
