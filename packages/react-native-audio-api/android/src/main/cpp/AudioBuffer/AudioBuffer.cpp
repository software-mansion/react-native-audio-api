#include "AudioBuffer.h"

namespace audioapi {

AudioBuffer::AudioBuffer(int numberOfChannels, int length, int sampleRate)
    : numberOfChannels_(numberOfChannels),
      length_(length),
      sampleRate_(sampleRate),
      duration_(static_cast<double>(length) / sampleRate) {
    if (numberOfChannels != 1 && numberOfChannels != 2) {
        throw std::invalid_argument("Invalid number of channels");
    }

    channels_ = static_cast<float **>(malloc(numberOfChannels * sizeof(float *)));

    for (int i = 0; i < numberOfChannels; i++) {
        channels_[i] = static_cast<float *>(malloc(length * sizeof(float)));

        for (int j = 0; j < length; j++) {
            channels_[i][j] = 0.0f;
        }
    }
}

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

  return channels_[channel];
}

void AudioBuffer::setChannelData(int channel, const float *data, int length) {
  if (channel < 0 || channel >= numberOfChannels_) {
    throw std::invalid_argument("Invalid channel number");
  }

  if (length != length_) {
    throw std::invalid_argument("Invalid data length");
  }

  memcpy(channels_[channel], data, length * sizeof(float));
}

std::shared_ptr<AudioBuffer> AudioBuffer::mix(int outputNumberOfChannels) {
  if (outputNumberOfChannels != 1 && outputNumberOfChannels != 2) {
    throw std::invalid_argument("Invalid number of channels");
  }

  if (outputNumberOfChannels == numberOfChannels_) {
    return shared_from_this();
  }

  auto mixedBuffer = std::make_shared<AudioBuffer>(outputNumberOfChannels, length_, sampleRate_);

  switch(this->numberOfChannels_) {
    case 1:
      mixedBuffer->setChannelData(0, this->channels_[0], length_);
        mixedBuffer->setChannelData(1, this->channels_[0], length_);
      break;
    case 2:
        for (int i = 0; i < length_; i++) {
            mixedBuffer->channels_[0][i] = (this->channels_[0][i] + this->channels_[1][i]) / 2;
        }
      break;
  }

  return mixedBuffer;
}
} // namespace audioapi
