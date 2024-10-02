#ifdef ANDROID
#include "AudioBufferWrapper.h"

namespace audioapi {

AudioBufferWrapper::AudioBufferWrapper(const std::shared_ptr<AudioBuffer> &audioBuffer)
    : audioBuffer_(audioBuffer) {
//  sampleRate = audioBuffer->getSampleRate();
//  length = audioBuffer->getLength();
//  duration = audioBuffer->getDuration();
//  numberOfChannels = audioBuffer->getNumberOfChannels();
}

int AudioBufferWrapper::getSampleRate() const {
  //return sampleRate;
    return 0;
}

int AudioBufferWrapper::getLength() const {
  //return length;
    return 0;
}

double AudioBufferWrapper::getDuration() const {
  //return duration;
    return 0;
}

int AudioBufferWrapper::getNumberOfChannels() const {
  //return numberOfChannels;
    return 0;
}

float *AudioBufferWrapper::getChannelData(int channel) const {
  //return audioBuffer_->getChannelData(channel);
    return nullptr;
}

void AudioBufferWrapper::setChannelData(int channel, float *data, int) const {
  //audioBuffer_->setChannelData(channel, data);
}
} // namespace audioapi
#endif
