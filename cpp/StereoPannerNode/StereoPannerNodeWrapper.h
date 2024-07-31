#pragma once

#include <memory>
#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"

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
        std::shared_ptr<StereoPannerNode> pannerNode_;
    public:
        explicit StereoPannerNodeWrapper(const std::shared_ptr<StereoPannerNode> &pannerNode);
#else
        public:
        explicit StereoPannerNodeWrapper() {}
#endif
    private:
        std::shared_ptr<AudioParamWrapper> panParam_;
    public:
        std::shared_ptr<AudioParamWrapper> getPanParam();
    };
} // namespace audiocontext
