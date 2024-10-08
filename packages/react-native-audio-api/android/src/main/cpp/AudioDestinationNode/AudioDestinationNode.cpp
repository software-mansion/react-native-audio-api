#include "AudioDestinationNode.h"
#include "AudioContext.h"

namespace audioapi {

AudioDestinationNode::AudioDestinationNode(AudioContext *context)
    : AudioNode(context) {
  numberOfOutputs_ = 0;
  numberOfInputs_ = 10;
  channelCountMode_ = ChannelCountMode::EXPLICIT;
}

const std::vector<float> &AudioDestinationNode::getOutputBuffer() {
    processAudio();
    return outputBuffer_;
}

void AudioDestinationNode::processAudio() {
    AudioNode::processAudio();

    auto maxValue = 1.0f;

    for (float i : inputBuffer_) {
        maxValue = std::max(maxValue, std::abs(i));
    }

    for (int i = 0; i < inputBuffer_.size(); i++) {
        outputBuffer_[i] = inputBuffer_[i] / maxValue;
    }
}

} // namespace audioapi
