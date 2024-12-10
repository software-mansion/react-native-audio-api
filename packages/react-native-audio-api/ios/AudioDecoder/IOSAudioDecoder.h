#pragma once

#include <string>

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioDecoder.h>
#else // when compiled as C++
typedef struct objc_object AudioDecoder;
#endif // __OBJC__

namespace audioapi {

class AudioBus;

class IOSAudioDecoder {
 protected:
  AudioDecoder *audioDecoder_;
  int sampleRate_;

 public:
  IOSAudioDecoder(int sampleRate);
  ~IOSAudioDecoder();

  AudioBus *decodeWithFilePath(const std::string &path);
  // TODO: implement this
  AudioBus *decodeWithArrayBuffer();
};
} // namespace audioapi
