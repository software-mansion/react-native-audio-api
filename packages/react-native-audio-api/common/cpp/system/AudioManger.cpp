#include "AudioManager.h"

#ifdef ANDROID

#else
#include "IOSAudioManagerBridge.h"
#endif // ANDROID

namespace audioapi {

AudioManager::AudioManager() {
#ifdef ANDROID

#else
  audioManagerBridge_ = std::make_shared<IOSAudioManagerBridge>();
#endif // ANDROID
}

#ifdef ANDROID

#else
std::shared_ptr<IOSAudioManagerBridge> & AudioManager::getIOSManagerBridge() {
  return audioManagerBridge_;
}
#endif // ANDROID

} // namespace audioapi
