#pragma once

#include <audioapi/core/AudioPlayer.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/types/AudioContextLatencyHint.h>
#include <audioapi/core/types/AudioContextOptions.h>
#include <audioapi/core/types/AudioTimestamp.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/jsi/ContextPromiseResolver.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <memory>

namespace audioapi {

class AudioContext : public BaseAudioContext {
 public:
  explicit AudioContext(
      float sampleRate,
      AndroidOutputProfile androidOutputProfile,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      AudioContextLatencyHint latencyHint);
  ~AudioContext() override;
  DELETE_COPY_AND_MOVE(AudioContext);

  void close(std::unique_ptr<ContextStateResolver> promise);
  bool resume(std::unique_ptr<ContextStateResolver> promise);
  bool suspend(std::unique_ptr<ContextStateResolver> promise);
  bool start();

  /// @brief Returns the base latency of the audio context in seconds.
  /// @returns The base latency in seconds.
  [[nodiscard]] double getBaseLatency() const;

  /// @brief Returns the output latency of the audio context in seconds.
  /// @returns The output latency in seconds.
  [[nodiscard]] double getOutputLatency() const;

  /// @brief Spec `getOutputTimestamp()`: the context time of the sample leaving the output device
  /// now, and now on the `performance.now()` clock. Both are zero before the first rendered block
  /// or where the platform cannot tell.
  /// @note JS Thread only.
  [[nodiscard]] AudioTimestamp getOutputTimestamp() const;

  /// @brief Called when the audio stream failed to rebuild.
  /// @note This method is called when the audio stream fails.
  void onStreamFail();

  void assignOnErrorCallbackId(uint64_t callbackId);

 private:
  AudioContextLatencyHint latencyHint_;
  AndroidOutputProfile androidOutputProfile_;
  std::atomic<bool> isInitialized_{false};
  /// Audio I/O callback thread increments around each platform render callback;
  /// control thread waits on suspend/close.
  std::atomic<uint32_t> currentRenders_{0};
  std::shared_ptr<AudioPlayer> audioPlayer_;
  /// Whether `audioPlayer_` holds this context for stream-failure reporting. Guarded by
  /// `driverMutex_`.
  bool isPlayerBound_ = false;

  EventCaller<AudioEvent::CONTEXT_ERROR> onErrorEvent_;

  bool isDriverRunning() const override;

  /// Caller must hold `driverMutex_`.
  bool tryStartDriver();

  /// Blocks until no audio I/O callback is in flight. Caller must hold `driverMutex_`.
  void waitForRenderQuiescence() const;

  /// Stops the player and releases the driver; shared by `close` and the destructor. Caller
  /// must hold `driverMutex_`.
  void stopDriver();
};

} // namespace audioapi
