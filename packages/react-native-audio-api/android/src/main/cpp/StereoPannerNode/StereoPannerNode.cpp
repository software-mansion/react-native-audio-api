#include "StereoPannerNode.h"

namespace audioapi {

StereoPannerNode::StereoPannerNode() {
  panParam_ = std::make_shared<AudioParam>(0.0, -1.0, 1.0);
}

std::shared_ptr<AudioParam> StereoPannerNode::getPanParam() const {
  return panParam_;
}

} // namespace audioapi
