#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/decoding/SeekDecoderDaemon.h>
#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
#include <audioapi/utils/events/PositionChangedDispatcher.h>
#include <cstddef>
#include <thread>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/miniaudio/MiniAudioDecoding.h>

#include <atomic>
#include <cstdint>
#include <memory>

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

  /// @brief Connects to @p node unless audio is routed through a media element.
  void connect(const std::shared_ptr<AudioNode> &node) override;

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

  void assignOnPositionChangedCallbackId(uint64_t callbackId);

 protected:
  /// @brief Outputs silence when media-routed; otherwise decodes into @p processingBuffer.
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioFileDecoderState> decoderState_;
  /// @brief Decodes and mixes samples for direct or media-element playback.
  /// @note Audio thread only.
  std::shared_ptr<DSPAudioBuffer> processDecodedOutput(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess);

  float volume_;
  bool filePaused_{false};
  bool loop_{false};
  double duration_{0};
  double sampleRate_{0};
  std::atomic<double> currentTime_{0};
  std::atomic<uint64_t> activeMediaBindingId_{0};

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
};

} // namespace audioapi
