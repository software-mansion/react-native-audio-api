#pragma once

#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"

#ifdef ANDROID
#include "StereoPannerNode.h"
#else
#include "IOSStereoPannerNode.h"
#include "IOSAudioContext.h"
#endif

namespace audiocontext {

#ifdef ANDROID
    class StereoPannerNode;
#endif

    class StereoPannerNodeWrapper: public AudioNodeWrapper {
#ifdef ANDROID
    public:
        explicit StereoPannerNodeWrapper(const std::shared_ptr<StereoPannerNode> &pannerNode);
#else
        private:
            std::shared_ptr<IOSStereoPannerNode> getStereoPannerNodeFromAudioNode();
        public:
            explicit StereoPannerNodeWrapper(std::shared_ptr<IOSAudioContext> context);
#endif
        private:
            std::shared_ptr<AudioParamWrapper> panParam_;
        public:
            std::shared_ptr<AudioParamWrapper> getPanParam();
    };
} // namespace audiocontext
