#include "StereoPannerNodeWrapper.h"

namespace audioapi {

StereoPannerNodeWrapper::StereoPannerNodeWrapper(
    const std::shared_ptr<StereoPannerNode> &stereoPannerNode)
    : AudioNodeWrapper(stereoPannerNode) {
  auto panParam = stereoPannerNode->getPanParam();
  panParam_ = std::make_shared<AudioParamWrapper>(panParam);
}

std::shared_ptr<AudioParamWrapper> StereoPannerNodeWrapper::getPanParam()
    const {
  return panParam_;
}
} // namespace audioapi
