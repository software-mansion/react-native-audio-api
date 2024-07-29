#pragma once

#include <memory>
#include "AudioNodeWrapper.h"

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
        private:
            std::shared_ptr<StereoPannerNode> panner_;
        public:
            explicit StereoPannerNodeWrapper(const std::shared_ptr<StereoPannerNode> &panner) : AudioNodeWrapper(
                    panner), panner_(panner) {}
#else
        private:
            std::shared_ptr<IOSStereoPannerNode> panner_;
        public:
            explicit StereoPannerNodeWrapper(std::shared_ptr<IOSAudioContext> context) : AudioNodeWrapper() {
                node_ = panner_ = std::make_shared<IOSStereoPannerNode>(context);
            }
#endif
            double getPan();
            void setPan(double pan);
    };
} // namespace audiocontext
