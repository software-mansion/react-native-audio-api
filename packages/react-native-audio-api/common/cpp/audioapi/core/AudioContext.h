#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>

#include <memory>
#include <functional>

namespace audioapi {
#ifdef ANDROID
class AudioPlayer;
#else
class IOSAudioPlayer;
#endif

class AudioContext : public BaseAudioContext {
 public:
  explicit AudioContext(float sampleRate, bool initSuspended, const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry, const RuntimeRegistry &runtimeRegistry);
  ~AudioContext() override;

  void close();
  bool resume();
  bool suspend();


 private:
#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif
  bool playerHasBeenStarted_;

  bool isDriverRunning() const override;

  std::function<void(std::shared_ptr<AudioBus>, int)> renderAudio();
};

} // namespace audioapi
