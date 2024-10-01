#pragma once

#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

class GainNode : public AudioNode {

public:
    explicit GainNode();

    std::shared_ptr<AudioParam> getGainParam() const;

private:
    std::shared_ptr<AudioParam> gainParam_;
};

} // namespace audioapi
