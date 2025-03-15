#pragma once

#include <memory>

namespace audioapi {

#ifdef ANDROID

#else
class IOSAudioManagerBridge;
#endif // ANDROID

class SessionOptions;

class AudioManager {
 public:
  AudioManager(AudioManager const &) = delete;
  void operator=(AudioManager const &) = delete;

  static AudioManager *getInstance();
  static void destroyInstance();

  void setSessionOptions(std::shared_ptr<SessionOptions> &sessionOptions);
  // std::shared_ptr<SessionOptions> getSessionOptions() const;

#ifdef ANDROID

#else
  std::shared_ptr<IOSAudioManagerBridge> &getIOSManagerBridge();
#endif // ANDROID

 private:
  AudioManager();

  static AudioManager *audioManager_;

#ifdef ANDROID

#else
  std::shared_ptr<IOSAudioManagerBridge> audioManagerBridge_;
#endif // ANDROID
};

} // namespace audioapi
