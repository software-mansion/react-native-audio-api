#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include "AudioNode.h"

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class OscillatorNode : public jni::HybridClass<OscillatorNode, AudioNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/oscillator/OscillatorNode;";

        void start(double time);

        void stop(double time);

        double getFrequency();

        double getDetune();

        std::string getWaveType();

        void setFrequency(double frequency);

        void setDetune(double detune);

        void setWaveType(const std::string &waveType);
    };
}// namespace audiocontext
