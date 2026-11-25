#pragma once

#include <audioapi/core/types/AudioContextLatencyHint.h>
#include <audioapi/core/types/AudioContextOptions.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/OutputTimestamp.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/TripleBuffer.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace audioapi {

class AudioContext;

class AudioPlayer {
 public:
  AudioPlayer(
      const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
      float sampleRate,
      int channelCount,
      std::atomic<uint32_t> &currentRenders,
      std::weak_ptr<AudioContext> context,
      std::mutex *driverMutex,
      AudioContextLatencyHint latencyHint)
      : renderAudio_(renderAudio),
        renderBuffer_(
            std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, channelCount, sampleRate)),
        sampleRate_(sampleRate),
        channelCount_(channelCount),
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

  /// @brief Which context frame leaves the output device when, or `std::nullopt` before the
  /// first rendered callback or on a platform that cannot tell.
  /// @note JS Thread only: it is the single reader of the anchors published from the audio thread.
  [[nodiscard]] virtual std::optional<OutputPresentation> getOutputPresentation() = 0;

 protected:
  /// One render callback's first output sample, tied to the platform's own timeline.
  struct OutputAnchor {
    /// Context frame of the callback's first output sample.
    std::int64_t contextFrame = 0;
    /// `steady_clock` time the platform attached to that sample, before output latency.
    std::int64_t hostTimeNs = 0;
    /// Index of that sample in the device stream, counted by platform.
    std::int64_t streamFrame = 0;
    /// Set by `publishOutputAnchor`: the `markStarted` count the anchor was published under.
    std::uint64_t startCount = 0;
    bool isValid = false;
  };

  /// @note Audio Thread only.
  void publishOutputAnchor(const OutputAnchor &anchor) {
    auto *slot = outputAnchors_.getForWriter();
    *slot = anchor;
    slot->startCount = startCount_.load(std::memory_order_acquire);
    outputAnchors_.publish();
  }

  /// @brief Call before every stream start/resume. Anchors published before it become stale:
  /// extrapolating one would span the pause, and after a rebuild its positions belong to a
  /// stream the new one does not continue.
  void markStarted() {
    startCount_.fetch_add(1, std::memory_order_release);
  }

  /// @note JS Thread only. `std::nullopt` until the current start publishes an anchor.
  [[nodiscard]] std::optional<OutputAnchor> latestOutputAnchor() {
    const OutputAnchor anchor = *outputAnchors_.getForReader();
    if (!anchor.isValid || anchor.startCount != startCount_.load(std::memory_order_acquire)) {
      return std::nullopt;
    }
    return anchor;
  }

  /// @brief Pulls `framesToProcess` frames from the graph into `renderBuffer_` and peak-normalizes
  /// them.
  /// @note Audio Thread only. The limiting lives in the player (not the destination node) so
  /// offline renders stay spec-accurate.
  void renderNormalizedQuantum(int framesToProcess) {
    renderAudio_(renderBuffer_.get(), framesToProcess);
    renderBuffer_->normalize();
    renderedFrames_ += framesToProcess;
  }

  std::function<void(DSPAudioBuffer *, int)> renderAudio_;
  std::shared_ptr<DSPAudioBuffer> renderBuffer_;
  float sampleRate_;
  int channelCount_;
  std::atomic<uint32_t> &currentRenders_;
  std::weak_ptr<AudioContext> context_;
  /// The owning context's driver mutex; `nullptr` disables stream-failure reporting.
  std::mutex *driverMutex_;
  std::atomic<bool> isRunning_{false};
  AudioContextLatencyHint latencyHint_;
  /// Frames pulled from the graph so far. The realtime context renders only through this player,
  /// and `processGraph` advances its frame counter by exactly these frames, so this equals the
  /// context frame of the next frame to render. Audio Thread only.
  std::int64_t renderedFrames_ = 0;

 private:
  TripleBuffer<OutputAnchor> outputAnchors_;
  std::atomic<std::uint64_t> startCount_{0};
};

/// Builds the platform's AudioPlayer.
std::shared_ptr<AudioPlayer> createPlatformAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    int channelCount,
    std::atomic<uint32_t> &currentRenders,
    std::weak_ptr<AudioContext> context,
    std::mutex *driverMutex,
    AudioContextLatencyHint latencyHint,
    AndroidOutputProfile outputProfile);

} // namespace audioapi
