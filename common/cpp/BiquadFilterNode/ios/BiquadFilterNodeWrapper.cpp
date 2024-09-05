#ifndef ANDROID
#include "BiquadFilterNodeWrapper.h"
namespace audioapi {

BiquadFilterNodeWrapper::BiquadFilterNodeWrapper(
    std::shared_ptr<IOSAudioContext> context)
    : AudioNodeWrapper() {
  node_ = std::make_shared<IOSBiquadFilterNode>(context);
  frequencyParam_ = std::make_shared<AudioParamWrapper>(
      getBiquadFilterNodeFromAudioNode()->getFrequencyParam());
  detuneParam_ = std::make_shared<AudioParamWrapper>(
      getBiquadFilterNodeFromAudioNode()->getDetuneParam());
  QParam_ = std::make_shared<AudioParamWrapper>(
      getBiquadFilterNodeFromAudioNode()->getQParam());
  gainParam_ = std::make_shared<AudioParamWrapper>(
      getBiquadFilterNodeFromAudioNode()->getGainParam());
}

std::shared_ptr<IOSBiquadFilterNode>
BiquadFilterNodeWrapper::getBiquadFilterNodeFromAudioNode() {
  return std::static_pointer_cast<IOSBiquadFilterNode>(node_);
}

std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getFrequencyParam()
    const {
  return frequencyParam_;
}

std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getDetuneParam()
    const {
  return detuneParam_;
}

std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getQParam() const {
  return QParam_;
}

std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getGainParam()
    const {
  return gainParam_;
}

std::string BiquadFilterNodeWrapper::getType() {
  return getBiquadFilterNodeFromAudioNode()->getType();
}

void BiquadFilterNodeWrapper::setType(const std::string &filterType) {
  getBiquadFilterNodeFromAudioNode()->setType(filterType);
}
} // namespace audioapi
#endif
