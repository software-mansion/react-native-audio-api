#ifdef ANDROID
#include "AudioBufferSourceNodeWrapper.h"

namespace audiocontext {

    AudioBufferSourceNode *AudioBufferSourceNodeWrapper::getAudioBufferSourceNodeFromAudioNode() {
        return dynamic_cast<AudioBufferSourceNode *>(node_);
    }

    void AudioBufferSourceNodeWrapper::start(double time) {
        auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
        audioBufferSourceNode->start(time);
    }

    void AudioBufferSourceNodeWrapper::stop(double time) {
        auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
        audioBufferSourceNode->stop(time);
    }

    bool AudioBufferSourceNodeWrapper::getLoop() {
        auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
        return audioBufferSourceNode->getLoop();
    }

    void AudioBufferSourceNodeWrapper::setLoop(bool loop) {
        auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
        audioBufferSourceNode->setLoop(loop);
    }

    std::shared_ptr<AudioBufferWrapper> AudioBufferSourceNodeWrapper::getBuffer() {
        auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
        auto buffer = audioBufferSourceNode->getBuffer();
        return std::make_shared<AudioBufferWrapper>(buffer);
    }
}
#endif
