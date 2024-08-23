#ifndef ANDROID
#include "OscillatorNodeWrapper.h"

namespace audiocontext
{
    OscillatorNodeWrapper::OscillatorNodeWrapper(std::shared_ptr<IOSAudioContext> context) : AudioNodeWrapper() {
        node_ = std::make_shared<IOSOscillatorNode>(context);
        frequencyParam_ = std::make_shared<AudioParamWrapper>(getOscillatorNodeFromAudioNode()->getFrequencyParam());
        detuneParam_ = std::make_shared<AudioParamWrapper>(getOscillatorNodeFromAudioNode()->getDetuneParam());
    }

    std::shared_ptr<IOSOscillatorNode> OscillatorNodeWrapper::getOscillatorNodeFromAudioNode() {
        return std::static_pointer_cast<IOSOscillatorNode>(node_);
    }

    std::string OscillatorNodeWrapper::getType() {
        return getOscillatorNodeFromAudioNode()->getType();
    }

    void OscillatorNodeWrapper::start(double time) {
        getOscillatorNodeFromAudioNode()->start(time);
    }

    void OscillatorNodeWrapper::stop(double time) {
        getOscillatorNodeFromAudioNode()->stop(time);
    }

    void OscillatorNodeWrapper::setType(const std::string& type) {
        getOscillatorNodeFromAudioNode()->setType(type);
    }

    std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getFrequencyParam() const {
        return frequencyParam_;
    }

    std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getDetuneParam() const {
        return detuneParam_;
    }
} // namespace audiocontext
#endif
