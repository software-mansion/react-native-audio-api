#ifdef ANDROID
#include "GainNodeWrapper.h"

namespace audiocontext {

    GainNodeWrapper::GainNodeWrapper(const std::shared_ptr<GainNode> &gainNode) : AudioNodeWrapper(
            gainNode) {
        auto gainParam = gainNode->getGainParam();
        gainParam_ = std::make_shared<AudioParamWrapper>(gainParam);
    }

    std::shared_ptr<AudioParamWrapper> GainNodeWrapper::getGainParam() {
        return gainParam_;
    }
} // namespace audiocontext
#endif
