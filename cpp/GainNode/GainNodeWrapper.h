#pragma once

#ifdef ANDROID
#else
#include "IOSGainNode.h"
#include "IOSAudioContext.h"
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
            explicit GainNodeWrapper(std::shared_ptr<IOSAudioContext> context) : AudioNodeWrapper() {
                node_ = gainNode_ = std::make_shared<IOSGainNode>(context);
            }
            void setGain(double gain);
            double getGain();
#endif
    };
} // namespace audiocontext
