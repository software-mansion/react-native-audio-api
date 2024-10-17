#include "GainNodeWrapper.h"

namespace audioapi {

GainNodeWrapper::GainNodeWrapper(const std::shared_ptr<GainNode> &gainNode)
    : AudioNodeWrapper(gainNode) {
  auto gainParam = gainNode->getGainParam();
  gainParam_ = std::make_shared<AudioParamWrapper>(gainParam);
}

std::shared_ptr<AudioParamWrapper> GainNodeWrapper::getGainParam() const {
  return gainParam_;
}
} // namespace audioapi
