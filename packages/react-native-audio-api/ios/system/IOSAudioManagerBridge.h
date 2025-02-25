#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <IOSAudioManager.h>
#else // when compiled as C++
typedef struct objc_object IOSAudioManager;
#endif // __OBJC__

#include <functional>

namespace audioapi {

class IOSAudioManagerBridge {
 protected:
  IOSAudioManager *iosAudioManager_;

 public:
  explicit IOSAudioManagerBridge();
  IOSAudioManager *getAudioManager();

  ~IOSAudioManagerBridge();
};

} // namespace audioapi
