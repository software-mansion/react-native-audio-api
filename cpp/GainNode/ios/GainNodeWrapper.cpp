#ifndef ANDROID
#include "GainNodeWrapper.h"

namespace audiocontext
{
    GainNodeWrapper::GainNodeWrapper(std::shared_ptr<IOSAudioContext> context) : AudioNodeWrapper() {
        node_ = std::make_shared<IOSGainNode>(context);
        gainParam_ = std::make_shared<AudioParamWrapper>(getGainNodeFromAudioNode()->getAudioParam());
    }

    std::shared_ptr<IOSGainNode> GainNodeWrapper::getGainNodeFromAudioNode() {
        return std::static_pointer_cast<IOSGainNode>(node_);
    }

    std::shared_ptr<AudioParamWrapper> GainNodeWrapper::getGainParam() {
        return gainParam_;
    }
} // namespace audiocontext
#endif
