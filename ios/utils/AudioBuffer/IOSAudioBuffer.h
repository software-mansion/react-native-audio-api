#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <RNAA_AudioBuffer.h>
#else // when compiled as C++
typedef struct objc_object RNAA_AudioBuffer;
#endif // __OBJC__

#import <stdexcept>

namespace audioapi {
class IOSAudioBuffer {
    
 public:
  RNAA_AudioBuffer *audioBuffer_;
    
public:

  IOSAudioBuffer(RNAA_AudioBuffer *audioBuffer);
  ~IOSAudioBuffer();
  int getSampleRate();
  int getLength();
  int getNumberOfChannels();
  float getDuration();
  float **getChannelData(int channel);
  void setChannelData(int channel, float **data, int length);
};
} // namespace audioapi
