#include "AudioManager.h"

#ifdef ANDROID

#else
#include "IOSAudioManagerBridge.h"
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
