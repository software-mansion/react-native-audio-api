#include "AudioDestinationNode.h"
#include "BaseAudioContext.h"
#include "VectorMath.h"

namespace audioapi {

AudioDestinationNode::AudioDestinationNode(BaseAudioContext *context)
    : AudioNode(context), currentSampleFrame_(0) {
  numberOfOutputs_ = 0;
  numberOfInputs_ = INT_MAX;
  channelCountMode_ = ChannelCountMode::EXPLICIT;
}

unsigned AudioDestinationNode::getCurrentSampleFrame() const {
  return currentSampleFrame_;
}

double AudioDestinationNode::getCurrentTime() const {
  return currentSampleFrame_ / context_->getSampleRate();
}

void AudioDestinationNode::renderAudio(AudioBus *destinationBus, int32_t numFrames) {
  // int32_t numSamples = numFrames * CHANNEL_COUNT;

  // memset(destinationBuffer, 0.0f, sizeof(float) * numSamples);

  // if (!numFrames) {
  //   return;
  // }

  // processAudio(numFrames);

  // currentSampleFrame_ += numFrames;
}

// bool AudioDestinationNode::processAudio(float *audioData, int32_t numFrames) {
//   for (auto &node : inputNodes_) {
//     if (node && node->processAudio(mixingBuffer.get(), numFrames)) {
//       normalize(mixingBuffer.get(), numFrames);
//       VectorMath::add(audioData, mixingBuffer.get(), audioData, numSamples);
//     }
//   }

//   return true;
// }

} // namespace audioapi
