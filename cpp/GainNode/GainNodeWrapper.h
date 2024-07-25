#pragma once

#ifdef ANDROID
#else
#include "IOSGainNode.h"
#endif

#include <memory>
#include "AudioNodeWrapper.h"

namespace audiocontext {
    class GainNodeWrapper: public AudioNodeWrapper {
        private:
#ifdef ANDROID
#else
            std::shared_ptr<IOSGainNode> gainNode_;
#endif

        public:
#ifdef ANDROID
            explicit GainNodeWrapper() {}
#else
            explicit GainNodeWrapper() : gainNode_(std::make_shared<IOSGainNode>()), AudioNodeWrapper() {
                node_ = gainNode_;
            }
            void setGain(double gain);
            double getGain();
#endif
    };
} // namespace audiocontext
