#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/WsolaTimeStretcher.h>
#include <audioapi/core/utils/decoding/SeekDecoderDaemon.h>
#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/events/PositionChangedDispatcher.h>
#include <cstddef>
#include <thread>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/miniaudio/MiniAudioDecoding.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>

using namespace audioapi::channels;

namespace audioapi {

struct AudioFileSourceOptions;
class MediaElementAudioSourceNode;

inline constexpr auto ON_POSITION_CHANGED_INTERVAL = 0.25f;

/// @brief Decodes a file or in-memory buffer and plays it as a scheduled source.
/// @note When routed through MediaElementAudioSourceNode, this node outputs silence and the media node pulls decoded audio.
class AudioFileSourceNode : public AudioScheduledSourceNode {
  friend class MediaElementAudioSourceNode;

 public:
  explicit AudioFileSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      AudioFileSourceOptions &options);
  ~AudioFileSourceNode() override;
  DELETE_COPY_AND_MOVE(AudioFileSourceNode);

  /// @brief Closes the decoder and tears down offloaded seek workers.
  void disable() override;

  /// @brief Schedules playback; auto-connects to the destination when not media-routed.
  void start(double when) override;

  /// @brief True while a media element owns decoding (active binding id is non-zero).
  [[nodiscard]] bool isRoutedThroughMediaElement() const {
    return activeMediaBindingId_.load(std::memory_order_acquire) != 0;
  }

  /// @brief Sends on position changed event.
  /// @param framesPlayed number of frames played since the last event.
  void sendOnPositionChangedEvent(int framesPlayed);

  /// @brief Registers @p bindingId as the current media-element owner of this decoder.
  void bindMediaElementSource(uint64_t bindingId);

  /// @brief Clears the binding when @p bindingId matches; resumes direct destination routing if playing.
  void releaseMediaElementSource(uint64_t bindingId);

  /// @brief True if @p bindingId is the active media-element binding.
  [[nodiscard]] bool isCurrentMediaElementSource(uint64_t bindingId) const;

  /// @brief Sets linear gain applied when writing decoded samples.
  void setVolume(float v) {
    volume_ = v;
  }

  /// @brief Sets time-stretched playback speed for decoded file sources.
  /// @note No-op for HLS live streams — playback rate changes are not supported.
  void setPlaybackRate(float v);

  /// @brief Current playback speed multiplier.
  float getPlaybackRate() const {
    return decoderState_->playbackRate.load(std::memory_order_acquire);
  }

  /// @brief True when the source is an FFmpeg HLS live/indefinite stream.
  [[nodiscard]] bool isHlsStreaming() const {
    return decoderState_ != nullptr &&
        decoderState_->isHlsStreaming.load(std::memory_order_acquire);
  }

  /// @brief When true, playback speed changes preserve the original pitch.
  void setPreservesPitch(bool v);

  /// @brief Current pitch-preservation mode.
  bool getPreservesPitch() const {
    return decoderState_->preservesPitch.load(std::memory_order_acquire);
  }

  /// @brief Stops decoding on the audio thread until playback is started again.
  void pause();

  /// @brief Enables looping at end-of-file.
  void setLoop(bool v) {
    loop_ = v;
    decoderState_->loop.store(v, std::memory_order_release);
  }

  /// @brief File duration in seconds (zero if unknown).
  double getDuration() const {
    return duration_;
  }

  /// @brief Current playback position in seconds.
  double getCurrentTime() const {
    return currentTime_.load(std::memory_order_acquire);
  }

  /// @brief Seeks asynchronously on a worker thread.
  void seekToTime(double seconds);

  /// @brief True when paused or stopped after natural end-of-file (non-looping).
  bool filePaused() const {
    return filePaused_;
  }

  bool canBeDestructed() const override {
    return filePaused() || AudioScheduledSourceNode::canBeDestructed();
  }

  void assignOnPositionChangedCallbackId(uint64_t callbackId);

 protected:
  void processNode(int framesToProcess) override;

 private:
  std::shared_ptr<AudioFileDecoderState> decoderState_;
  /// @brief Decodes and mixes samples for direct or media-element playback.
  /// @param outputBuffer when non-null, decoded audio is written here instead of this node's buffer.
  /// @note Audio thread only.
  void processDecodedOutput(
      int framesToProcess,
      const std::shared_ptr<DSPAudioBuffer> &outputBuffer = nullptr);

  /// @brief Handles the paused / drained / stopped short-circuit states before decoding.
  /// @return a finished output buffer when the render should bail out early, std::nullopt otherwise.
  /// @note Audio thread only. On success @p outContext holds a locked context.
  std::optional<std::shared_ptr<DSPAudioBuffer>> renderPreflight(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess,
      std::shared_ptr<BaseAudioContext> &outContext);

  float volume_;
  WsolaTimeStretcher wsolaStretcher_;
  std::shared_ptr<DSPAudioBuffer> playbackRateBuffer_;
  bool filePaused_{false};
  bool loop_{false};
  double duration_{0};
  double sampleRate_{0};
  std::atomic<double> currentTime_{0};
  std::atomic<uint64_t> activeMediaBindingId_{0};

  // Target rate from JS; applied immediately on the audio thread (Chromium does not smooth rate).
  float targetPlaybackRate_{1.0f};
  static constexpr float RATE_SETTLE_EPSILON = 1e-2f;

  enum class FillMode : std::uint8_t { Passthrough, Resample, Wsola };
  FillMode lastFillMode_{FillMode::Passthrough};
  std::array<float, MAX_CHANNEL_COUNT> previousOutputFrame_{};
  bool hasPreviousOutputFrame_{false};
  bool endOfStreamStopPending_{false};
  bool endOfStreamDrainPending_{false};
  float eofDrainRate_{1.0f};
  size_t transitionFadeFramesRemaining_{0};
  static constexpr size_t MODE_TRANSITION_FADE_FRAMES = RENDER_QUANTUM_SIZE;

  [[nodiscard]] static FillMode chooseFillMode(bool preservesPitch, bool rateAffectsOutput);
  void setFillMode(FillMode mode);
  void applyModeTransitionFade(const std::shared_ptr<DSPAudioBuffer> &processingBuffer);
  void applyFadeOutToSilence(const std::shared_ptr<DSPAudioBuffer> &processingBuffer);
  void captureLastOutputFrame(const std::shared_ptr<DSPAudioBuffer> &processingBuffer);
  std::shared_ptr<DSPAudioBuffer> handleEndOfStreamPlayback(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess,
      float activeRate);

  PositionChangedDispatcher positionChanged_;

  /// @brief Sets up SPSC channels, constructs the SeekDecoderDaemon, and initialises metadata from the opened decoder.
  /// @return false if the source could not be opened; caller must not set isInitialized_.
  [[nodiscard]] bool initDecoder(
      const std::shared_ptr<BaseAudioContext> &context,
      AudioFileSourceOptions &options);

  /// @brief Attempts to read the next chunk of decoded frames from the daemon.
  /// @param outData decoded frames and metadata; only valid if return value is true.
  /// @return false if no decoded frames are available; true if @p outData is filled and ready to process.
  [[nodiscard]] bool readNextFrameChunk(DecoderData &outData);

  /// @brief Clears decoded chunks generated with stale playback settings.
  /// @note Audio thread only. Used when toggling pitch-preservation mode.
  void drainPendingFrames();

  /// @brief Resets stretch/resample state after pitch-preservation mode changes.
  /// @note Audio thread only.
  void handlePitchPreservationModeChanged();

  /// @brief Ensures the planar playback-rate buffer can hold @p frames.
  /// @note Audio thread only.
  bool ensurePlaybackRateBufferSize(size_t frames);

  /// @brief Appends deinterleaved samples from interleaved PCM into @ref playbackRateBuffer_.
  /// @return Number of frames copied from @p interleaved; 0 if nothing to copy.
  /// @note Audio thread only.
  size_t appendFromInterleaved(
      const float *interleaved,
      size_t frameCount,
      size_t startFrame,
      size_t framesNeeded,
      size_t &totalInputFrames);

  /// @brief Renders a speed-changed chunk while keeping pitch unchanged.
  /// @note Audio thread only.
  /// @return Number of source frames consumed from the decoder pipeline.
  size_t renderWithWsolaPitchPreservation(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      DecoderData &incoming,
      int framesToProcess,
      float activeRate);

  /// @brief Pulls enough decoded PCM for one WSOLA/resample quantum (may read multiple SPSC chunks).
  /// @return Number of input frames accumulated, or 0 on failure.
  /// @note Audio thread only.
  size_t accumulateStretchInput(
      DecoderData &incoming,
      float activeRate,
      int framesToProcess,
      size_t minFramesNeeded = 0);

  /// @brief Drops any partially consumed decoder chunk retained for sub-1.0x playback.
  /// @note Audio thread only.
  void clearPendingDecoderChunk();

  /// @brief Retains the unconsumed tail of @p chunk for the next render quantum.
  /// @note Audio thread only.
  void stashPendingDecoderChunk(const DecoderData &chunk, size_t consumedFrames);

  /// @brief Removes @p consumedFrames from the front of the pending chunk buffer.
  /// @note Audio thread only.
  void consumePendingDecoderChunkFront(size_t consumedFrames);

  /// @brief Resets pitch-preservation state.
  /// @note Audio thread only.
  void resetPitchPreservationState();

  /// @brief Renders a speed-changed chunk with pitch following playback speed.
  /// @note Audio thread only.
  /// @return Number of source frames consumed from the decoder pipeline.
  size_t renderWithoutPitchPreservation(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      DecoderData &incoming,
      int framesToProcess,
      float playbackRate);

  /// @brief Daemon thread for decoding and seeking
  std::unique_ptr<SeekDecoderDaemon> seekDecoderDaemon_;
  std::thread seekDecoderThread_;

  /// @brief Signals the daemon to stop and joins its thread. Idempotent; safe
  /// to call from both disable() and the destructor.
  void stopDaemonThread();

  /// @brief Connects to the destination when leaving media routing while playback is active.
  /// @note Audio thread only.
  void ensureConnectedForDirectPlayback();

  /// @brief SPSC for JS -> Daemon thread communication (seek event)
  CommandSender commandSender_;

  /// @brief SPSC for Daemon thread -> Audio thread communication (decoded frames)
  std::shared_ptr<FrameReceiver> frameReceiver_;

  /// @brief Tail of a decoder chunk not yet consumed (needed when playbackRate < 1.0).
  DecoderData pendingDecoderChunk_;
};

} // namespace audioapi
