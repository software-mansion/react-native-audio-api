#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <CAudioRecorder.h>
#else // when compiled as C++
typedef struct objc_object CAudioRecorder;
#endif // __OBJC__

#include <functional>

namespace audioapi {

class IOSAudioRecorder {
 protected:
  CAudioRecorder *audioRecorder_;

 public:
  IOSAudioRecorder();
  ~IOSAudioRecorder();

  void start();
  void stop();
};

} // namespace audioapi
