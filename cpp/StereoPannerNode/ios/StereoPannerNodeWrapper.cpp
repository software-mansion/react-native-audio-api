#ifndef ANDROID
#include "StereoPannerNodeWrapper.h"

namespace audiocontext {
    void StereoPannerNodeWrapper::setPan(double pan) {
        panner_->setPan(pan);
    }

    double StereoPannerNodeWrapper::getPan() {
        return panner_->getPan();
    }
} // namespace audiocontext

#endif
