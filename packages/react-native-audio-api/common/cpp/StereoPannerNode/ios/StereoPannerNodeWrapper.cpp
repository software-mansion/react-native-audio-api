#ifndef ANDROID
#include "StereoPannerNodeWrapper.h"

namespace audioapi {

StereoPannerNodeWrapper::StereoPannerNodeWrapper(
    const std::shared_ptr<IOSStereoPannerNode> &stereoPannerNode)
    : AudioNodeWrapper(stereoPannerNode) {
  panParam_ =
      std::make_shared<AudioParamWrapper>(stereoPannerNode->getPanParam());
}

std::shared_ptr<AudioParamWrapper> StereoPannerNodeWrapper::getPanParam()
    const {
  return panParam_;
}
} // namespace audioapi
#endif
