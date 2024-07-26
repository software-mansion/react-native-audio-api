#pragma once

#include <memory>
#include "AudioNodeWrapper.h"
#include "GainNodeWrapper.h"

#ifdef ANDROID
#include "OscillatorNode.h"
#else
#include "IOSOscillator.h"
#include "IOSAudioContext.h"
#endif

namespace audiocontext {
#ifdef ANDROID
    class OscillatorNode;
#endif

    class OscillatorNodeWrapper: public AudioNodeWrapper {
    private:
#ifdef ANDROID
        std::shared_ptr<OscillatorNode> oscillator_;
#else
        std::shared_ptr<IOSOscillator> oscillator_;
#endif

    public:
#ifdef ANDROID
    explicit OscillatorNodeWrapper( std::shared_ptr<OscillatorNode> oscillator) : AudioNodeWrapper(
            oscillator), oscillator_(oscillator) {}
#else
        explicit OscillatorNodeWrapper(std::shared_ptr<IOSAudioContext> context) : AudioNodeWrapper() {
            node_ = oscillator_ = std::make_shared<IOSOscillator>(context);
        }
#endif
        double getFrequency();
        double getDetune();
        std::string getType();
        void start(double time);
        void stop(double time);

        void setFrequency(double frequency);
        void setDetune(double detune);
        void setType(const std::string& type);
    };
} // namespace audiocontext
