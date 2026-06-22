#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <functional>
#include <memory>
#include <mutex>

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
  DELETE_COPY_AND_MOVE(AudioContext);

  /// Promise thread pool (`AudioContextHostObject::close`).
  void close();
  /// Promise thread pool (`AudioContextHostObject::resume`). Races with `start()`.
  bool resume();
  /// Promise thread pool (`AudioContextHostObject::suspend`). Races with `start()`.
  bool suspend();
  /// JS thread (`AudioScheduledSourceNode::start`). Races with `resume` / `suspend` / `close`.
  bool start();
  /// JS thread only — runs synchronously in `BaseAudioContextHostObject` construction.
  void initialize() override;

  /// JS thread only — synchronous HostObject call; graph mutation only.
  std::shared_ptr<MediaElementAudioSourceNode> createMediaElementSource(
      const std::shared_ptr<AudioFileSourceNode> &fileSource);

 private:
#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif
  /// Serializes `start` / `resume` / `suspend` / `close` across JS and promise-pool threads.
  mutable std::mutex driverMutex_;
  std::atomic<bool> isInitialized_{false};

  /// JS thread only — read from `getState()`; must not acquire `driverMutex_`.
  bool isDriverRunning() const override;

  std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> renderAudio();

  /// Caller must hold `driverMutex_`.
  bool tryStartDriver();
};

} // namespace audioapi
