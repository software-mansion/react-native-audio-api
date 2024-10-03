#ifdef ANDROID
#include "AudioBufferWrapper.h"

namespace audioapi {

AudioBufferWrapper::AudioBufferWrapper(
    const std::shared_ptr<AudioBuffer> &audioBuffer)
    : audioBuffer_(audioBuffer) {
    sampleRate = audioBuffer->getSampleRate();
    length = audioBuffer->getLength();
    duration = audioBuffer->getDuration();
    numberOfChannels = audioBuffer->getNumberOfChannels();
}

int AudioBufferWrapper::getSampleRate() const {
  return sampleRate;
}

int AudioBufferWrapper::getLength() const {
  return length;
}

double AudioBufferWrapper::getDuration() const {
  return duration;
}

int AudioBufferWrapper::getNumberOfChannels() const {
  return numberOfChannels;
}

float *AudioBufferWrapper::getChannelData(int channel) const {
  return audioBuffer_->getChannelData(channel);
}

void AudioBufferWrapper::setChannelData(int channel, float *data, int length) const {
  audioBuffer_->setChannelData(channel, data, length);
}
} // namespace audioapi
#endif
