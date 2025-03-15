#include <audioapi/system/AudioManager.h>
#include <audioapi/system/SessionOptions.h>

#ifdef ANDROID

#else
#include <audioapi/ios/system/IOSAudioManagerBridge.h>
#endif // ANDROID

namespace audioapi {

AudioManager *AudioManager::audioManager_ = nullptr;

AudioManager *AudioManager::getInstance() {
  if (audioManager_ == nullptr) {
    audioManager_ = new AudioManager();
  }

  return audioManager_;
}

void AudioManager::destroyInstance() {
  delete audioManager_;
  audioManager_ = nullptr;
}

void AudioManager::setSessionOptions(
    std::shared_ptr<SessionOptions> &sessionOptions) {
#ifdef ANDROID

#else
  audioManagerBridge_->setSessionOptions(sessionOptions);
#endif
}

// std::shared_ptr<SessionOptions> AudioManager::getSessionOptions() const {
// #ifdef ANDROID

// #else
//   return audioManagerBridge_->getSessionOptions();
// #endif
// }

AudioManager::AudioManager() {
#ifdef ANDROID

#else
  audioManagerBridge_ = std::make_shared<IOSAudioManagerBridge>();
#endif // ANDROID
}

#ifdef ANDROID

#else
std::shared_ptr<IOSAudioManagerBridge> &AudioManager::getIOSManagerBridge() {
  return audioManagerBridge_;
}
#endif // ANDROID

} // namespace audioapi
