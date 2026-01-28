#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>

#include <functional>
#include <memory>

namespace audioapi {
#ifdef ANDROID
class AudioPlayer;
#else
class IOSAudioPlayer;
#endif

class AudioContext : public BaseAudioContext {
 public:
  explicit AudioContext(
      float sampleRate,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const RuntimeRegistry &runtimeRegistry);
  ~AudioContext() override;

  /// @note JS Thread only
  void close();
  /// @note JS Thread only
  bool resume();
  /// @note JS Thread only
  bool suspend();
  /// @note JS Thread only
  bool start();
  /// @note JS Thread only
  void initialize() override;

 private:
#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif
  bool isInitialized_;

  bool isDriverRunning() const override;

  std::function<void(std::shared_ptr<AudioBus>, int)> renderAudio();
};

} // namespace audioapi
