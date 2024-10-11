#include "AudioDestinationNode.h"
#include "AudioContext.h"

namespace audioapi {

AudioDestinationNode::AudioDestinationNode(AudioContext *context)
    : AudioNode(context) {
  numberOfOutputs_ = 0;
  numberOfInputs_ = 10;
  channelCountMode_ = ChannelCountMode::EXPLICIT;
}

void AudioDestinationNode::renderAudio(float *audioData, int32_t numFrames) {
  processAudio(audioData, numFrames);
}

bool AudioDestinationNode::processAudio(float *audioData, int32_t numFrames) {
  int numSamples = numFrames * channelCount_;

  if (mixingBuffer == nullptr) {
    mixingBuffer = std::make_unique<float[]>(numSamples);
  }

  memset(audioData, 0.0f, sizeof(float) * numSamples);

  for (auto &node : inputNodes_) {
    if (node->processAudio(mixingBuffer.get(), numFrames)) {
      for (int i = 0; i < numSamples; i++) {
        audioData[i] += mixingBuffer[i];
      }
    }
  }

  return true;
}

} // namespace audioapi
