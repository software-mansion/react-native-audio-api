#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include "AudioNode.h"
#include "AudioParam.h"

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class OscillatorNode : public jni::HybridClass<OscillatorNode, AudioNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/oscillator/OscillatorNode;";

        void start(double time);
        void stop(double time);
        std::shared_ptr<AudioParam> getFrequencyParam();
        std::shared_ptr<AudioParam> getDetuneParam();
        std::string getWaveType();
        void setWaveType(const std::string &waveType);
        void prepareForDeconstruction();
    };
}// namespace audiocontext
