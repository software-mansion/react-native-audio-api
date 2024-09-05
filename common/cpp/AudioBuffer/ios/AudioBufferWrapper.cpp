#ifndef ANDROID
#include "AudioBufferWrapper.h"

namespace audioapi {

// TODO: Implement the AudioParamWrapper class

int AudioBufferWrapper::getSampleRate() const {
  return 1;
}

int AudioBufferWrapper::getLength() const {
  return 1;
}

double AudioBufferWrapper::getDuration() const {
  return 1.0;
}

int AudioBufferWrapper::getNumberOfChannels() const {
  return 1;
}

int16_t **AudioBufferWrapper::getChannelData(int channel) const {
  return nullptr;
}

void AudioBufferWrapper::setChannelData(int channel, int16_t **data) {
  return;
}

} // namespace audioapi
#endif
