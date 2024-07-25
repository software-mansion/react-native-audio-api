#pragma once

#include <memory>
#include "AudioNodeWrapper.h"

#ifdef ANDROID
#include "OscillatorNode.h"
#else
#include "IOSOscillator.h"
#endif

namespace audiocontext {
#ifdef ANDROID
    class OscillatorNode;
#endif

    class OscillatorNodeWrapper: public AudioNodeWrapper {
#ifdef ANDROID
        std::shared_ptr<OscillatorNode> oscillator_;
    public:
        explicit OscillatorNodeWrapper(const std::shared_ptr<OscillatorNode> &oscillator) : AudioNodeWrapper(
                oscillator), oscillator_(oscillator) {}
#else
        explicit OscillatorNodeWrapper() : oscillator_(std::make_shared<IOSOscillator>()) {}
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
