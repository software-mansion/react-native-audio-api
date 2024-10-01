#include "GainNode.h"

namespace audioapi {

GainNode::GainNode() {
  gainParam_ = std::make_shared<AudioParam>(1.0, 0.0, 1.0);
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
    buffer[i] *= gainParam_->getValue();
  }

  AudioNode::process(oboeStream, audioData, numFrames, channelCount);
}

} // namespace audioapi
