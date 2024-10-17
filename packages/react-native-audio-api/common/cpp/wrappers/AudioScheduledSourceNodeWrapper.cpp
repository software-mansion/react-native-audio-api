#include "AudioScheduledSourceNodeWrapper.h"

namespace audioapi {

AudioScheduledSourceNodeWrapper::AudioScheduledSourceNodeWrapper(
    const std::shared_ptr<AudioScheduledSourceNode> &audioScheduledSourceNode)
    : AudioNodeWrapper(audioScheduledSourceNode) {}

std::shared_ptr<AudioScheduledSourceNode>
AudioScheduledSourceNodeWrapper::getAudioScheduledSourceNodeFromAudioNode() {
  return std::static_pointer_cast<AudioScheduledSourceNode>(node_);
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
