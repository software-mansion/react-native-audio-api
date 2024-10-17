#include "GainNode.h"
#include "AudioContext.h"

namespace audioapi {

GainNode::GainNode(AudioContext *context) : AudioNode(context) {
  gainParam_ = std::make_shared<AudioParam>(context, 1.0, -MAX_GAIN, MAX_GAIN);
}

std::shared_ptr<AudioParam> GainNode::getGainParam() const {
  return gainParam_;
}

bool GainNode::processAudio(float *audioData, int32_t numFrames) {
  if (!AudioNode::processAudio(audioData, numFrames)) {
    return false;
  }

  auto time = context_->getCurrentTime();
  auto deltaTime = 1.0 / context_->getSampleRate();

  for (int i = 0; i < numFrames * channelCount_; i++) {
    audioData[i] *= gainParam_->getValueAtTime(time);
    time += deltaTime;
  }

  return true;
}

} // namespace audioapi
