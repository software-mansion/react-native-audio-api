#include "AudioBuffer.h"

namespace audioapi {

AudioBuffer::AudioBuffer(int numberOfChannels, int length, int sampleRate)
    : numberOfChannels_(numberOfChannels),
      length_(length),
      sampleRate_(sampleRate),
      duration_(static_cast<double>(length) / sampleRate),
      channels_(numberOfChannels, std::vector<float>(length, 0.0f)) {}

int AudioBuffer::getNumberOfChannels() const {
  return numberOfChannels_;
}

int AudioBuffer::getLength() const {
  return length_;
}

int AudioBuffer::getSampleRate() const {
  return sampleRate_;
}

double AudioBuffer::getDuration() const {
  return duration_;
}

float *AudioBuffer::getChannelData(int channel) const {
  if (channel < 0 || channel >= numberOfChannels_) {
    throw std::invalid_argument("Invalid channel number");
  }

  auto channelData = (float *)malloc(length_ * sizeof(float));
  for (int i = 0; i < length_; i++) {
    channelData[i] = channels_[channel][i];
  }

  return channelData;
}

void AudioBuffer::setChannelData(int channel, const float *data, int length) {
  if (channel < 0 || channel >= numberOfChannels_) {
    throw std::invalid_argument("Invalid channel number");
  }

  if (length != length_) {
    throw std::invalid_argument("Invalid data length");
  }

  for (int i = 0; i < length_; i++) {
    channels_[channel][i] = data[i];
  }
}
} // namespace audioapi
