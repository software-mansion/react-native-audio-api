#ifndef ANDROID
#include "StereoPannerNodeWrapper.h"

namespace audiocontext {

    double StereoPannerNodeWrapper::getPan() {
        throw std::runtime_error("[StereoPannerHostObject::getPan] Not yet implemented!");
    }

    void StereoPannerNodeWrapper::setPan(double pan) {
        throw std::runtime_error("[StereoPannerHostObject::setPan] Not yet implemented!");
    }
} // namespace audiocontext

#endif
