#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <IOSAudioManager.h>
#else // when compiled as C++
typedef struct objc_object IOSAudioManager;
#endif // __OBJC__

#include <functional>
#include <memory>

namespace audioapi {
class SessionOptions;

class IOSAudioManagerBridge {
 protected:
  IOSAudioManager *iosAudioManager_;

 public:
  explicit IOSAudioManagerBridge();
  IOSAudioManager *getAudioManager();

  void setSessionOptions(std::shared_ptr<SessionOptions> &sessionOptions);
  //  std::shared_ptr<SessionOptions> getSessionOptions() const;

  ~IOSAudioManagerBridge();
};

} // namespace audioapi
