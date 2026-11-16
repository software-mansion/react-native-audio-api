#pragma once

#include <audioapi/core/AudioPlayer.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/types/AudioContextLatencyHint.h>
#include <audioapi/core/types/AudioContextOptions.h>
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

  void close(const std::shared_ptr<ContextStateResolver> &promise);
  bool resume(const std::shared_ptr<ContextStateResolver> &promise);
  bool suspend(const std::shared_ptr<ContextStateResolver> &promise);
  bool start();

  /// @brief Initializes native audio player and assigns the audio destination node to the context.
  /// @param destination The audio destination node to be associated with the context.
  /// @note This method must be called before the audio context can be used for processing audio.
  void initialize(const AudioDestinationNode *destination) final;

  /// @brief Returns the base latency of the audio context in seconds.
  /// @returns The base latency in seconds.
  [[nodiscard]] double getBaseLatency() const;

  /// @brief Returns the output latency of the audio context in seconds.
  /// @returns The output latency in seconds.
  [[nodiscard]] double getOutputLatency() const;

  /// @brief Called when the audio stream failed to rebuild.
  /// @note This method is called when the audio stream fails.
  void onStreamFail();

  /// @brief Restarts the stream of a context that is meant to be running but whose stream the
  /// system stopped, e.g. by an audio session interruption. Any other context is left alone.
  /// @return false only when such a context could not restart its stream.
  /// @note Takes `driverMutex_`, so the caller must not hold it.
  bool resumeAfterInterruption();

  /// @brief Suspends a context that is meant to be running, after the system stopped its stream
  /// and playback should not continue. Any other context is left alone.
  /// @note Publishes `suspended` with a `statechange`, since no JS promise carries this
  /// transition. Unlike `onStreamFail`, the context stays usable: `resume()` restarts the stream.
  /// Takes `driverMutex_`, so the caller must not hold it.
  void suspendAfterInterruption();

  void assignOnErrorCallbackId(uint64_t callbackId);

 private:
  std::shared_ptr<AudioPlayer> audioPlayer_;
  AudioContextLatencyHint latencyHint_;
  AndroidOutputProfile androidOutputProfile_;
  std::atomic<bool> isInitialized_{false};
  /// Audio I/O callback thread increments around each platform render callback;
  /// control thread waits on suspend/close.
  std::atomic<uint32_t> currentRenders_{0};

  EventCaller<AudioEvent::CONTEXT_ERROR> onErrorEvent_;

  bool isDriverRunning() const override;

  /// Caller must hold `driverMutex_`.
  bool tryStartDriver();

  /// Blocks until no audio I/O callback is in flight. Caller must hold `driverMutex_`.
  void waitForRenderQuiescence() const;

  /// Stops the player and releases the driver; shared by `close` and the destructor. Caller
  /// must hold `driverMutex_`.
  void stopDriver();

  /// Stops rendering and hands graph draining back to the producer. Does not change the state.
  /// Caller must hold `driverMutex_`.
  void suspendDriver();

  /// The state set by the last acknowledged `resume()`, `suspend()` or driver start. Unlike
  /// `getState()`, it does not report `suspended` because the driver happens to be stopped.
  [[nodiscard]] ContextState getControlState() const {
    return state_.load(std::memory_order_acquire);
  }
};

} // namespace audioapi
