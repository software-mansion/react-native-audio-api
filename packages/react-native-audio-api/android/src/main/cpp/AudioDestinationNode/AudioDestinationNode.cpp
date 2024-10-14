#include "AudioDestinationNode.h"
#include "AudioContext.h"

namespace audioapi {

AudioDestinationNode::AudioDestinationNode(AudioContext *context, int32_t numFrames)
    : AudioNode(context) {
  numberOfOutputs_ = 0;
  numberOfInputs_ = INT_MAX;
  channelCountMode_ = ChannelCountMode::EXPLICIT;
  mixingBuffer = std::make_unique<float[]>(numFrames * CHANNEL_COUNT);
}

void AudioDestinationNode::renderAudio(float *audioData, int32_t numFrames) {
  processAudio(audioData, numFrames);
}

bool AudioDestinationNode::processAudio(float *audioData, int32_t numFrames) {
  int numSamples = numFrames * CHANNEL_COUNT;

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
