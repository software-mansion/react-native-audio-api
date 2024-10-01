#ifdef ANDROID
#include "OscillatorNodeWrapper.h"

namespace audioapi {

std::shared_ptr<OscillatorNode>
OscillatorNodeWrapper::getOscillatorNodeFromAudioNode() {
  return std::static_pointer_cast<OscillatorNode>(node_);
}

OscillatorNodeWrapper::OscillatorNodeWrapper(
    const std::shared_ptr<OscillatorNode> &oscillator)
    : AudioScheduledSourceNodeWrapper(oscillator) {
  //  auto frequencyParam = oscillator->getFrequencyParam();
  //  frequencyParam_ = std::make_shared<AudioParamWrapper>(frequencyParam);
  //  auto detuneParam = oscillator->getDetuneParam();
  //  detuneParam_ = std::make_shared<AudioParamWrapper>(detuneParam);
}

std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getFrequencyParam()
    const {
  return frequencyParam_;
}

std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getDetuneParam()
    const {
  return detuneParam_;
}

std::string OscillatorNodeWrapper::getType() {
  auto oscillatorNode_ = getOscillatorNodeFromAudioNode();
  // return oscillatorNode_->getWaveType();
  return "";
}

void OscillatorNodeWrapper::setType(const std::string &type) {
  auto oscillatorNode_ = getOscillatorNodeFromAudioNode();
  // oscillatorNode_->setWaveType(type);
}
} // namespace audioapi
#endif
