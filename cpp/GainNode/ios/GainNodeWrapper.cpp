#ifndef ANDROID
#include "GainNodeWrapper.h"

namespace audiocontext
{
    GainNodeWrapper::GainNodeWrapper(std::shared_ptr<IOSAudioContext> context) : AudioNodeWrapper() {
        node_ = gain_ = std::make_shared<IOSGainNode>(context);
        gainParam_ = std::make_shared<AudioParamWrapper>(gain_->getAudioParam());
    }

    std::shared_ptr<AudioParamWrapper> GainNodeWrapper::getGainParam() {
        return gainParam_;
    }
} // namespace audiocontext
#endif
