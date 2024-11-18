#include "AudioBus.h"
#include "AudioNode.h"
#include "VectorMath.h"
#include "BaseAudioContext.h"
#include "AudioDestinationNode.h"

namespace audioapi {

AudioDestinationNode::AudioDestinationNode(BaseAudioContext *context)
    : AudioNode(context), currentSampleFrame_(0) {
  numberOfOutputs_ = 0;
  numberOfInputs_ = INT_MAX;
  channelCountMode_ = ChannelCountMode::EXPLICIT;
}

std::size_t AudioDestinationNode::getCurrentSampleFrame() const {
  return currentSampleFrame_;
}

double AudioDestinationNode::getCurrentTime() const {
  return static_cast<double>(currentSampleFrame_) / context_->getSampleRate();
}

void AudioDestinationNode::renderAudio(AudioBus *destinationBus, int32_t numFrames) {
  destinationBus->zero();

  if (!numFrames) {
    return;
  }

  processAudio(destinationBus, numFrames);
  currentSampleFrame_ += numFrames;
}

} // namespace audioapi
