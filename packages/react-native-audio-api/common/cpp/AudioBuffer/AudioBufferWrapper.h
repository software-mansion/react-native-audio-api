#pragma once

#include <memory>

#ifdef ANDROID
#include "AudioBuffer.h"
#else
#include "IOSAudioBuffer.h"
#endif

namespace audioapi {

class AudioBufferWrapper {
#ifdef ANDROID

 public:
  AudioBuffer *audioBuffer_;

  explicit AudioBufferWrapper(AudioBuffer *audioBuffer);
  ~AudioBufferWrapper();
#else

 public:
  std::shared_ptr<IOSAudioBuffer> audioBuffer_;

  explicit AudioBufferWrapper(std::shared_ptr<IOSAudioBuffer> audioBuffer);
#endif

 private:
    int numberOfChannels;
  int length;
        int sampleRate;
  double duration;

 public:
        int getNumberOfChannels() const;
  int getLength() const;
  double getDuration() const;
        int getSampleRate() const;
  float *getChannelData(int channel) const;
  void setChannelData(int channel, float *data, int length) const;
};
} // namespace audioapi
