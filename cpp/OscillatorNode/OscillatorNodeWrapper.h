#pragma once

#include <memory>
#include "AudioDestinationNodeWrapper.h"

#ifdef ANDROID
#include "OscillatorNode.h"
#else
#include "IOSOscillator.h"
#endif

namespace audiocontext {

    class AudioDestinationNodeWrapper;

#ifdef ANDROID
    class OscillatorNode;
#endif

    class OscillatorNodeWrapper {
    private:
#ifdef ANDROID
        std::shared_ptr<OscillatorNode> oscillator_;
#else
        std::shared_ptr<IOSOscillator> oscillator_;
#endif

    public:
#ifdef ANDROID
        explicit OscillatorNodeWrapper(std::shared_ptr<OscillatorNode> oscillator) : oscillator_(oscillator) {}
#else
        explicit OscillatorNodeWrapper() : oscillator_(std::make_shared<IOSOscillator>()) {}
#endif
        double getFrequency();
        double getDetune();
        std::string getType();
        void start(double time);
        void stop(double time);
        void connect(std::shared_ptr<AudioDestinationNodeWrapper> destination);

        void setFrequency(double frequency);
        void setDetune(double detune);
        void setType(std::string type);

    };
} // namespace audiocontext
