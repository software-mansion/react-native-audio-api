#ifdef ANDROID
#include "AudioBufferWrapper.h"

namespace audiocontext{

    AudioBufferWrapper::AudioBufferWrapper(AudioBuffer *audioBuffer) {
        audioBuffer_ = audioBuffer;
        sampleRate = audioBuffer->getSampleRate();
        length = audioBuffer->getLength();
        duration = audioBuffer->getDuration();
        numberOfChannels = audioBuffer->getNumberOfChannels();
    }

    AudioBufferWrapper::~AudioBufferWrapper() {
        audioBuffer_->prepareForDeconstruction();
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

    short** AudioBufferWrapper::getChannelData(int channel) const {
        return audioBuffer_->getChannelData(channel);
    }
}
#endif
