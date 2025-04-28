#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <NativeIOSAudioRecorder.h>
#else // when compiled as C++
typedef struct objc_object NativeIOSAudioRecorder;
#endif // __OBJC__

#include <functional>

namespace audioapi {

class AudioBus;

class IOSAudioRecorder {
 public:
  IOSAudioRecorder(
      float sampleRate,
      int numberOfChannels,
      int bufferLength,
      bool enableVoiceProcessing,
      const std::function<void(std::shared_ptr<AudioBus>, int, double)>
          &onAudioReady);

  ~IOSAudioRecorder();

  void start();
  void stop();

 protected:
  NativeIOSAudioRecorder *audioRecorder_;
  std::function<void(std::shared_ptr<AudioBus>, int, double)> onAudioReady_;
  std::atomic<bool> isRunning_;
};

} // namespace audioapi
