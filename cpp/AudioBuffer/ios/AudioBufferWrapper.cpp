#ifndef ANDROID
#include "AudioBufferWrapper.h"

namespace audiocontext {

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

short **AudioBufferWrapper::getChannelData(int channel) const {
  return nullptr;
}

void AudioBufferWrapper::setChannelData(int channel, short **data) {
  return;
}

} // namespace audiocontext
#endif
