#ifndef ANDROID
#include "AudioBufferWrapper.h"

namespace audioapi {

AudioBufferWrapper::AudioBufferWrapper(
    std::shared_ptr<IOSAudioBuffer> audioBuffer) {
  audioBuffer_ = audioBuffer;
  sampleRate = audioBuffer->getSampleRate();
  length = audioBuffer->getLength();
  duration = audioBuffer->getDuration();
  numberOfChannels = audioBuffer->getNumberOfChannels();
}

int AudioBufferWrapper::getNumberOfChannels() const {
  return numberOfChannels;
}

int AudioBufferWrapper::getLength() const {
  return length;
}

double AudioBufferWrapper::getDuration() const {
  return duration;
}

int AudioBufferWrapper::getSampleRate() const {
  return sampleRate;
}

float *AudioBufferWrapper::getChannelData(int channel) const {
  return audioBuffer_->getChannelData(channel);
}

void AudioBufferWrapper::setChannelData(int channel, float *data, int length)
    const {
  audioBuffer_->setChannelData(channel, data, length);
}

} // namespace audioapi
#endif
