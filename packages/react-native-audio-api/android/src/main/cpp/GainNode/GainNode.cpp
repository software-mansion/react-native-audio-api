#include "GainNode.h"
#include "AudioContext.h"

namespace audioapi {

GainNode::GainNode(AudioContext *context) : AudioNode(context) {
  gainParam_ = std::make_shared<AudioParam>(context, 1.0, 0.0, 1.0);
}

std::shared_ptr<AudioParam> GainNode::getGainParam() const {
  return gainParam_;
}

void GainNode::process(
    oboe::AudioStream *oboeStream,
    void *audioData,
    int32_t numFrames,
    int channelCount) {
  auto *buffer = static_cast<float *>(audioData);
  for (int i = 0; i < numFrames * channelCount; i++) {
    buffer[i] *= gainParam_->getValueAtTime(context_->getCurrentTime());
  }

  AudioNode::process(oboeStream, audioData, numFrames, channelCount);
}

} // namespace audioapi
