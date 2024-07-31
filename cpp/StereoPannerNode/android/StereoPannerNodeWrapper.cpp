#ifdef ANDROID
#include "StereoPannerNodeWrapper.h"

namespace audiocontext {

    StereoPannerNodeWrapper::StereoPannerNodeWrapper(const std::shared_ptr<StereoPannerNode> &pannerNode) : AudioNodeWrapper(
            pannerNode), pannerNode_(pannerNode) {
        auto panParam = pannerNode_->getPanParam();
        panParam_ = std::make_shared<AudioParamWrapper>(panParam);
    }

    std::shared_ptr<AudioParamWrapper> StereoPannerNodeWrapper::getPanParam() {
        return panParam_;
    }
} // namespace audiocontext
#endif
