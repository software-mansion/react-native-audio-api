#pragma once

#include "AudioNode.h"
#include "AudioBuffer.h"

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class AudioBufferSourceNode : public jni::HybridClass<AudioBufferSourceNode, AudioNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/AudioBufferSourceNode;";

        void start(double time);
        void stop(double time);
        bool getLoop();
        void setLoop(bool loop);
        AudioBuffer* getBuffer();
        void setBuffer(const AudioBuffer* buffer);
    };
}// namespace audiocontext
