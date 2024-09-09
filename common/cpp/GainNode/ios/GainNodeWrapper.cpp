#ifndef ANDROID
#include "GainNodeWrapper.h"

namespace audioapi {
GainNodeWrapper::GainNodeWrapper(std::shared_ptr<IOSGainNode> gainNode)
    : AudioNodeWrapper(gainNode) {
  gainParam_ = std::make_shared<AudioParamWrapper>(gainNode->getGainParam());
}

std::shared_ptr<IOSGainNode> GainNodeWrapper::getGainNodeFromAudioNode() {
  return std::static_pointer_cast<IOSGainNode>(node_);
}

std::shared_ptr<AudioParamWrapper> GainNodeWrapper::getGainParam() const {
  return gainParam_;
}
} // namespace audioapi
#endif
