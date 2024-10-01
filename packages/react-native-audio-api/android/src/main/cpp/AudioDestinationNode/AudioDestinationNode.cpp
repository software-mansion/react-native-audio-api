#include "AudioDestinationNode.h"

namespace audioapi {

AudioDestinationNode::AudioDestinationNode() : AudioNode() {
  numberOfOutputs_ = 0;
  channelCountMode_ = "explicit";
}

void AudioDestinationNode::process(
    AudioStream *oboeStream,
    void *audioData,
    int32_t numFrames,
    int channelCount) {
  normalize(audioData, numFrames, channelCount);
}

void AudioDestinationNode::normalize(
    void *audioData,
    int32_t numFrames,
    int channelCount) {
  auto *outputBuffer = static_cast<float *>(audioData);
  auto maxValue = 1.0f;

  for (int i = 0; i < numFrames * channelCount; i++) {
    maxValue = std::max(maxValue, std::abs(outputBuffer[i]));
  }

  std::for_each(
      outputBuffer,
      outputBuffer + numFrames * channelCount,
      [maxValue](float &sample) { sample /= maxValue; });
}

} // namespace audioapi
