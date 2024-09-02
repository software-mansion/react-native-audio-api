#pragma once

#include "AudioNodeWrapper.h"
#include "AudioBufferWrapper.h"

#ifdef ANDROID
#include "AudioBufferSourceNode.h"
#else
#include "IOSAudioBufferSourceNode.h"
#include "IOSAudioContext.h"
#endif

namespace audiocontext {

#ifdef ANDROID
    class AudioBufferSourceNode;
#endif

    class AudioBufferSourceNodeWrapper: public AudioNodeWrapper {
#ifdef ANDROID
    private:
        AudioBufferSourceNode* getAudioBufferSourceNodeFromAudioNode();
    public:
        explicit AudioBufferSourceNodeWrapper(AudioBufferSourceNode *audioBufferNode) : AudioNodeWrapper(audioBufferNode) {}
#else
        public:
        explicit AudioBufferSourceNodeWrapper() : AudioNodeWrapper() {
            node_ = std::make_shared<AudioNode>(context);
        }
#endif
    public:
        void start(double time);
        void stop(double time);
        void setLoop(bool loop);
        bool getLoop();
        std::shared_ptr<AudioBufferWrapper> getBuffer();
    };
} // namespace audiocontext
