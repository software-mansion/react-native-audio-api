#pragma once

#include <memory>
#include "AudioNodeWrapper.h"

#ifdef ANDROID
#include "GainNode.h"
#else
#include "IOSGainNode.h"
#include "IOSAudioContext.h"
#endif

namespace audiocontext {

#ifdef ANDROID
    class GainNode;
#endif

    class GainNodeWrapper: public AudioNodeWrapper {
#ifdef ANDROID
    private:
        std::shared_ptr<GainNode> gain_;
    public:
        explicit GainNodeWrapper(const std::shared_ptr<GainNode> &gain) : AudioNodeWrapper(
                gain), gain_(gain) {}
#else
    private:
        std::shared_ptr<IOSGainNode> gain_;
    public:
        explicit GainNodeWrapper(std::shared_ptr<IOSAudioContext> context) : AudioNodeWrapper() {
                node_ = gain_ = std::make_shared<IOSGainNode>(context);
            }
#endif
        double getGain();
        void setGain(double gain);
    };
} // namespace audiocontext
