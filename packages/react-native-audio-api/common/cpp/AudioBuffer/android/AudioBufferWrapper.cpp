#ifdef ANDROID
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

void AudioBufferWrapper::setChannelData(int channel, float *data, int length) const {
  audioBuffer_->setChannelData(channel, data, length);
}
} // namespace audioapi
#endif
