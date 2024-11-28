#include "AudioBuffer.h"
#include "AudioArray.h"
#include "AudioBus.h"

namespace audioapi {

AudioBuffer::AudioBuffer(int numberOfChannels, int length, int sampleRate) {
  bus_ = std::make_shared<AudioBus>(sampleRate, length, numberOfChannels);
}

int AudioBuffer::getLength() const {
  return bus_->getSize();
}

int AudioBuffer::getNumberOfChannels() const {
  return bus_->getNumberOfChannels();
}

int AudioBuffer::getSampleRate() const {
  return bus_->getSampleRate();
}

double AudioBuffer::getDuration() const {
  return static_cast<double>(getLength()) / getSampleRate();
}

float *AudioBuffer::getChannelData(int channel) const {
  return bus_->getChannel(channel)->getData();
}

void AudioBuffer::copyFromChannel(
    float *destination,
    int destinationLength,
    int channelNumber,
    int startInChannel) const {
  memcpy(
      destination,
      bus_->getChannel(channelNumber)->getData() + startInChannel,
      std::min(destinationLength, getLength() - startInChannel) *
          sizeof(float));
}

void AudioBuffer::copyToChannel(
    const float *source,
    int sourceLength,
    int channelNumber,
    int startInChannel) {
  memcpy(
      bus_->getChannel(channelNumber)->getData() + startInChannel,
      source,
      std::min(sourceLength, getLength() - startInChannel) * sizeof(float));
}

} // namespace audioapi
