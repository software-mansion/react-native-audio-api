#include "AudioBufferWrapper.h"

namespace audioapi {

AudioBufferWrapper::AudioBufferWrapper(
    const std::shared_ptr<AudioBuffer> &audioBuffer)
    : audioBuffer_(audioBuffer) {}

int AudioBufferWrapper::getSampleRate() const {
  return audioBuffer_->getSampleRate();
}

int AudioBufferWrapper::getLength() const {
  return audioBuffer_->getLength();
}

double AudioBufferWrapper::getDuration() const {
  return audioBuffer_->getDuration();
}

int AudioBufferWrapper::getNumberOfChannels() const {
  return audioBuffer_->getNumberOfChannels();
}

float *AudioBufferWrapper::getChannelData(int channel) const {
  return audioBuffer_->getChannelData(channel);
}

void AudioBufferWrapper::copyFromChannel(
    float *destination,
    int destinationLength,
    int channelNumber,
    int startInChannel) const {
  audioBuffer_->copyFromChannel(
      destination, destinationLength, channelNumber, startInChannel);
}

void AudioBufferWrapper::copyToChannel(
    float *source,
    int sourceLength,
    int channelNumber,
    int startInChannel) const {
  audioBuffer_->copyToChannel(
      source, sourceLength, channelNumber, startInChannel);
}
} // namespace audioapi
