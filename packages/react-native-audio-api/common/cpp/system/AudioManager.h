#pragma once

#include <memory>


namespace audioapi {

#ifdef ANDROID

#else
class IOSAudioManagerBridge;
#endif // ANDROID

class AudioManager {
 public:
  AudioManager();

#ifdef ANDROID

#else
  std::shared_ptr<IOSAudioManagerBridge> &getIOSManagerBridge();
#endif // ANDROID

 private:
#ifdef ANDROID

#else
  std::shared_ptr<IOSAudioManagerBridge> audioManagerBridge_;
#endif // ANDROID
};

} // namespace audioapi
