#pragma once

#include <audioapi/core/types/AudioContextLatencyHint.h>
#include <audioapi/core/types/AudioContextOptions.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace audioapi {

class AudioContext;

class AudioPlayer {
 public:
  AudioPlayer(
      const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
      float sampleRate,
      int channelCount,
      int renderQuantumSize,
      std::atomic<uint32_t> &currentRenders,
      std::weak_ptr<AudioContext> context,
      std::mutex *driverMutex,
      AudioContextLatencyHint latencyHint)
      : renderAudio_(renderAudio),
        renderBuffer_(
            std::make_shared<DSPAudioBuffer>(renderQuantumSize, channelCount, sampleRate)),
        sampleRate_(sampleRate),
        channelCount_(channelCount),
        renderQuantumSize_(renderQuantumSize),
        currentRenders_(currentRenders),
        context_(std::move(context)),
        driverMutex_(driverMutex),
        latencyHint_(latencyHint) {}
  DELETE_COPY_AND_MOVE(AudioPlayer);
  virtual ~AudioPlayer() = default;

  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual bool resume() = 0;
  virtual void suspend() = 0;
  virtual void cleanup() = 0;

  [[nodiscard]] virtual double getBaseLatency() const = 0;
  [[nodiscard]] virtual double getOutputLatency() const = 0;

  [[nodiscard]] virtual bool isRunning() const = 0;

 protected:
  /// @brief Pulls `framesToProcess` frames from the graph into `renderBuffer_` and peak-normalizes
  /// them.
  /// @note Audio Thread only. The limiting lives in the player (not the destination node) so
  /// offline renders stay spec-accurate.
  void renderNormalizedQuantum(int framesToProcess) {
    renderAudio_(renderBuffer_.get(), framesToProcess);
    renderBuffer_->normalize();
  }

  std::function<void(DSPAudioBuffer *, int)> renderAudio_;
  std::shared_ptr<DSPAudioBuffer> renderBuffer_;
  float sampleRate_;
  int channelCount_;
  /// Frames pulled from the graph per render call; sizes `renderBuffer_`.
  int renderQuantumSize_;
  std::atomic<uint32_t> &currentRenders_;
  std::weak_ptr<AudioContext> context_;
  /// The owning context's driver mutex; `nullptr` disables stream-failure reporting.
  std::mutex *driverMutex_;
  std::atomic<bool> isRunning_{false};
  AudioContextLatencyHint latencyHint_;
};

/// Builds the platform's AudioPlayer.
std::shared_ptr<AudioPlayer> createPlatformAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    int channelCount,
    int renderQuantumSize,
    std::atomic<uint32_t> &currentRenders,
    std::weak_ptr<AudioContext> context,
    std::mutex *driverMutex,
    AudioContextLatencyHint latencyHint,
    AndroidOutputProfile outputProfile);

} // namespace audioapi
