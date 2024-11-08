#include "AudioDestinationNode.h"
#include "BaseAudioContext.h"

namespace audioapi {

AudioDestinationNode::AudioDestinationNode(BaseAudioContext *context)
    : AudioNode(context) {
  numberOfOutputs_ = 0;
  numberOfInputs_ = INT_MAX;
  channelCountMode_ = ChannelCountMode::EXPLICIT;
}

void AudioDestinationNode::renderAudio(float *audioData, int32_t numFrames) {
  // processAudio(audioData, numFrames);
}

// bool AudioDestinationNode::processAudio(float *audioData, int32_t numFrames) {
//   int numSamples = numFrames * CHANNEL_COUNT;

//   if (mixingBuffer == nullptr) {
//     mixingBuffer = std::make_unique<float[]>(numSamples);
//   }

//   memset(audioData, 0.0f, sizeof(float) * numSamples);

//   for (auto &node : inputNodes_) {
//     if (node && node->processAudio(mixingBuffer.get(), numFrames)) {
//       normalize(mixingBuffer.get(), numFrames);
//       VectorMath::add(audioData, mixingBuffer.get(), audioData, numSamples);
//     }
//   }

//   return true;
// }

void AudioDestinationNode::normalize(float *audioData, int32_t numFrames) {
  auto maxValue = std::max(
      1.0f, VectorMath::maximumMagnitude(audioData, numFrames * channelCount_));

  if (maxValue == 1.0f) {
    return;
  }

  VectorMath::multiplyByScalar(
      audioData, 1.0f / maxValue, audioData, numFrames * channelCount_);
}

} // namespace audioapi
