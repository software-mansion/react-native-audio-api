#include "GainNode.h"
#include "AudioContext.h"

namespace audioapi {

GainNode::GainNode(AudioContext *context) : AudioNode(context) {
  gainParam_ = std::make_shared<AudioParam>(context, 1.0, -MAX_GAIN, MAX_GAIN);
}

std::shared_ptr<AudioParam> GainNode::getGainParam() const {
  return gainParam_;
}

void GainNode::processAudio() {
  AudioNode::processAudio();

  for (int i = 0; i < inputBuffer_.size(); i++) {
    outputBuffer_[i] = inputBuffer_[i] * gainParam_->getValueAtTime(context_->getCurrentTime());
  }
}
} // namespace audioapi
