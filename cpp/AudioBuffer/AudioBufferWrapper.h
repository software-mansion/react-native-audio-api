#pragma once

#include <memory>

#ifdef ANDROID
#include "AudioBuffer.h"
#else
#endif

namespace audiocontext {

    class AudioBufferWrapper {
#ifdef ANDROID
    protected:
        AudioBuffer* audioBuffer_;
    public:
        explicit AudioBufferWrapper(AudioBuffer *audioBuffer);
        ~AudioBufferWrapper();
#else
        protected:

        public:
            explicit AudioBufferWrapper() {}
#endif
    private:
        int sampleRate;
        int length;
        double duration;
        int numberOfChannels;
    public:
        int getSampleRate() const;
        int getLength() const;
        double getDuration() const;
        int getNumberOfChannels() const;
        short** getChannelData(int channel) const;

    };
} // namespace audiocontext
