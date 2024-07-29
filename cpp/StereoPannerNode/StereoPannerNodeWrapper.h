#pragma once

#include <memory>
#include "AudioNodeWrapper.h"

#ifdef ANDROID
#include "StereoPannerNode.h"
#endif

namespace audiocontext {

#ifdef ANDROID
    class StereoPannerNode;
#endif

    class StereoPannerNodeWrapper: public AudioNodeWrapper {
#ifdef ANDROID
    private:
        std::shared_ptr<StereoPannerNode> panner_;
    public:
        explicit StereoPannerNodeWrapper(const std::shared_ptr<StereoPannerNode> &panner) : AudioNodeWrapper(
                panner), panner_(panner) {}
#else
        public:
        explicit StereoPannerNodeWrapper() {}
#endif
        double getPan();
        void setPan(double pan);
    };
} // namespace audiocontext
