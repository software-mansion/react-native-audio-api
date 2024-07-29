#ifdef ANDROID
#include "StereoPannerNodeWrapper.h"

namespace audiocontext {

    double StereoPannerNodeWrapper::getPan() {
        return panner_->getPan();
    }

    void StereoPannerNodeWrapper::setPan(double pan) {
        panner_->setPan(pan);
    }
} // namespace audiocontext

#endif
