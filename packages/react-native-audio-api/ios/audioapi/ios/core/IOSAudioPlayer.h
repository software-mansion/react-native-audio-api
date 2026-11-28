#pragma once

#import <AVFoundation/AVFoundation.h>
#import <audioapi/ios/system/AudioEngine.h>

#include <audioapi/core/AudioPlayer.h>
#include <audioapi/core/types/AudioContextLatencyHint.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

namespace audioapi {

class IOSAudioPlayer : public AudioPlayer {
 public:
  IOSAudioPlayer(
      const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
      float sampleRate,
      std::atomic<uint32_t> &currentRenders,
      std::mutex *driverMutex,
      AudioContextLatencyHint latencyHint);
  ~IOSAudioPlayer() override;

  DELETE_COPY_AND_MOVE(IOSAudioPlayer);

  bool start() override;
  void stop() override;
  bool resume() override;
  void suspend() override;
  void cleanup() override;

  [[nodiscard]] bool isRunning() const override;

  [[nodiscard]] double getBaseLatency() const override;
  [[nodiscard]] double getOutputLatency() const override;

  /// The host time CoreAudio attaches to a buffer is when its first frame leaves the I/O
  /// unit; the session's `outputLatency` covers the rest of the way to the speaker.
  [[nodiscard]] std::optional<OutputPresentation> getOutputPresentation() override;

  /// Mach uptime: the clock React Native's `performance.now()` reads on iOS, and the one
  /// CoreAudio stamps buffers with. libc++ `steady_clock` (`CLOCK_MONOTONIC_RAW`) keeps counting
  /// while the device sleeps, so it runs ahead of both by the total sleep since boot.
  [[nodiscard]] std::int64_t presentationClockNowNs() const override;

 private:
  void clearPendingSaved();
  /// @note Audio Thread only
  /// Always pulls the graph in steps of RENDER_QUANTUM_SIZE; if the system
  /// buffer size is not a multiple of 128, the unused tail of the last quantum
  /// is kept (max 128 frames) and played at the start of the next callback.
  /// Publishes the context frame of the buffer's first sample against `timestamp`'s host time.
  void
  deliverOutputBuffers(AudioBufferList *outputData, int numFrames, const AudioTimeStamp *timestamp);

  /// In frames of the session's own rate, which may differ from the context's.
  int preferredIOBufferFrames_;
  AVAudioSourceNodeRenderBlock renderBlock_;
  /// The attached source node's id; nil while detached.
  NSString *sourceNodeId_;
  /// Set from main thread on start/resume; consumed on audio thread to drop stale pending audio.
  std::atomic<bool> flushOverflowNextPull_{false};
  /// Frames valid at the front of each `pendingSaved_[ch]` (0 … RENDER_QUANTUM_SIZE).
  int pendingSavedCount_{0};
  DSPAudioBuffer pendingSaved_{RENDER_QUANTUM_SIZE, DEFAULT_CHANNEL_COUNT, sampleRate_};
};

} // namespace audioapi
