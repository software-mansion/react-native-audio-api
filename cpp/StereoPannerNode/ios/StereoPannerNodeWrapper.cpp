#ifndef ANDROID
#include "StereoPannerNodeWrapper.h"

namespace audiocontext
{
    StereoPannerNodeWrapper::StereoPannerNodeWrapper(std::shared_ptr<IOSAudioContext> context) : AudioNodeWrapper() {
        node_ = std::make_shared<IOSStereoPannerNode>(context);
        panParam_ = std::make_shared<AudioParamWrapper>(getStereoPannerNodeFromAudioNode()->getPanParam());
    }

    std::shared_ptr<IOSStereoPannerNode> StereoPannerNodeWrapper::getStereoPannerNodeFromAudioNode() {
        return std::static_pointer_cast<IOSStereoPannerNode>(node_);
    }

    std::shared_ptr<AudioParamWrapper> StereoPannerNodeWrapper::getPanParam() {
        return panParam_;
    }
} // namespace audiocontext
#endif
