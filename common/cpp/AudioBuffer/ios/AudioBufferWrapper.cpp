#ifndef ANDROID
#include "AudioBufferWrapper.h"

namespace audioapi {

AudioBufferWrapper::AudioBufferWrapper(std::shared_ptr<IOSAudioBuffer> buffer) {
    audioBuffer_ = buffer;
}

int AudioBufferWrapper::getSampleRate() const {
    audioBuffer_->getSampleRate();
}

int AudioBufferWrapper::getLength() const {
    audioBuffer_->getLength();
}

double AudioBufferWrapper::getDuration() const {
    audioBuffer_->getDuration();
}

int AudioBufferWrapper::getNumberOfChannels() const {
    audioBuffer_->getNumberOfChannels();
}

float **AudioBufferWrapper::getChannelData(int channel) const {
  return nullptr;
}

void AudioBufferWrapper::setChannelData(int channel, float **data) const {
  return;
}

} // namespace audioapi
#endif
