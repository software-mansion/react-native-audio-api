#ifdef ANDROID
#include "AudioScheduledSourceNodeWrapper.h"

namespace audioapi {

    AudioScheduledSourceNode *
    AudioScheduledSourceNodeWrapper::getAudioScheduledSourceNodeFromAudioNode() {
        return static_cast<AudioScheduledSourceNode *>(node_);
    }

    void AudioScheduledSourceNodeWrapper::start(double time) {
        auto audioBufferSourceNode = getAudioScheduledSourceNodeFromAudioNode();
        audioBufferSourceNode->start(time);
    }

    void AudioScheduledSourceNodeWrapper::stop(double time) {
        auto audioBufferSourceNode = getAudioScheduledSourceNodeFromAudioNode();
        audioBufferSourceNode->stop(time);
    }
} // namespace audioapi
#endif
