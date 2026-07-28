#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/CommonPlayer.h>
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
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry);
  ~AudioContext() override;
  DELETE_COPY_AND_MOVE(AudioContext);

  void close(const std::shared_ptr<ContextPromiseResolver<>> &promise = nullptr);
  bool resume(const std::shared_ptr<ContextPromiseResolver<>> &promise);
  bool suspend(const std::shared_ptr<ContextPromiseResolver<>> &promise);
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

 private:
  std::shared_ptr<CommonPlayer> audioPlayer_;
  std::atomic<bool> isInitialized_{false};
  /// Audio I/O callback thread increments around each platform render callback;
  /// control thread waits on suspend/close.
  std::atomic<uint32_t> currentRenders_{0};

  bool isDriverRunning() const override;

  /// Caller must hold `driverMutex_`.
  bool tryStartDriver();

  /// Blocks until no audio I/O callback is in flight. Caller must hold `driverMutex_`.
  void waitForRenderQuiescence() const;
};

} // namespace audioapi
