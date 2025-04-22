#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <CAudioRecorder.h>
#else // when compiled as C++
typedef struct objc_object CAudioRecorder;
#endif // __OBJC__

#include <functional>

namespace audioapi {

class AudioBus;

class IOSAudioRecorder {
 protected:
  CAudioRecorder *audioRecorder_;

  std::function<void(std::shared_ptr<AudioBus>, int, double)> onAudioReady_;

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
};

} // namespace audioapi
