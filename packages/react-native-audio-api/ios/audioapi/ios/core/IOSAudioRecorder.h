#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <NativeAudioRecorder.h>
#else // when compiled as C++
typedef struct objc_object NativeAudioRecorder;
#endif // __OBJC__

#include <audioapi/core/inputs/AudioRecorder.h>

namespace audioapi {

class AudioBus;
class CircularAudioArray;

class IOSAudioRecorder : public AudioRecorder {
 public:
  IOSAudioRecorder(
      float sampleRate,
      int bufferLength,
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry);

  ~IOSAudioRecorder() override;

  void start() override;
  void stop() override;

 private:
  NativeAudioRecorder *audioRecorder_;
};

} // namespace audioapi
