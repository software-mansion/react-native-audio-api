#include "GainNode.h"

namespace audioapi {

    GainNode::GainNode() {
        gainParam_ = std::make_shared<AudioParam>(1.0, 0.0, 1.0);
    }

    std::shared_ptr<AudioParam> GainNode::getGainParam() const {
        return gainParam_;
    }

} // namespace audioapi
