#ifndef ANDROID
#include "StereoPannerNodeWrapper.h"

namespace audiocontext
{
    StereoPannerNodeWrapper::StereoPannerNodeWrapper(std::shared_ptr<IOSAudioContext> context) : AudioNodeWrapper() {
        node_ = pannerNode_ = std::make_shared<IOSStereoPannerNode>(context);
        panParam_ = std::make_shared<AudioParamWrapper>(pannerNode_->getAudioParam());
    }

    std::shared_ptr<AudioParamWrapper> StereoPannerNodeWrapper::getPanParam() const {
        return panParam_;
    }
} // namespace audiocontext
#endif
