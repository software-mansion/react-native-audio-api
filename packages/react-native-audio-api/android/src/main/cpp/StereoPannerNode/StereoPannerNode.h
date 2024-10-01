#pragma once

#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

class StereoPannerNode : public AudioNode {

public:
    explicit StereoPannerNode();
    std::shared_ptr<AudioParam> getPanParam() const;

private:
    std::shared_ptr<AudioParam> panParam_;
};

} // namespace audioapi
