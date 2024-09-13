#ifndef ANDROID
#include "AudioScheduledSourceNodeWrapper.h"

namespace audioapi {

std::shared_ptr<IOSAudioScheduledSourceNode> AudioScheduledSourceNodeWrapper::getAudioScheduledSourceNodeFromAudioNode() {
    return std::static_pointer_cast<IOSAudioScheduledSourceNode>(node_);
}

    void AudioScheduledSourceNodeWrapper::start(double time) {
        auto audioScheduledSourceNode = getAudioScheduledSourceNodeFromAudioNode();
        audioScheduledSourceNode->start(time);
    }

    void AudioScheduledSourceNodeWrapper::stop(double time) {
        auto audioScheduledSourceNode = getAudioScheduledSourceNodeFromAudioNode();
        audioScheduledSourceNode->stop(time);
    }
} // namespace audioapi
#endif
