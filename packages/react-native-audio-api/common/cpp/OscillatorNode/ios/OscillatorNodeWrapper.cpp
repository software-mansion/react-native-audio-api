#ifndef ANDROID
#include "OscillatorNodeWrapper.h"

namespace audioapi {
OscillatorNodeWrapper::OscillatorNodeWrapper(
    std::shared_ptr<IOSOscillatorNode> oscillatorNode)
    : AudioScheduledSourceNodeWrapper(oscillatorNode) {
  frequencyParam_ =
      std::make_shared<AudioParamWrapper>(oscillatorNode->getFrequencyParam());
  detuneParam_ =
      std::make_shared<AudioParamWrapper>(oscillatorNode->getDetuneParam());
}

std::shared_ptr<IOSOscillatorNode>
OscillatorNodeWrapper::getOscillatorNodeFromAudioNode() {
  return std::static_pointer_cast<IOSOscillatorNode>(node_);
}

std::string OscillatorNodeWrapper::getType() {
  return getOscillatorNodeFromAudioNode()->getType();
}

void OscillatorNodeWrapper::setType(const std::string &type) {
  getOscillatorNodeFromAudioNode()->setType(type);
}

std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getFrequencyParam()
    const {
  return frequencyParam_;
}

std::shared_ptr<AudioParamWrapper> OscillatorNodeWrapper::getDetuneParam()
    const {
  return detuneParam_;
}
} // namespace audioapi
#endif
