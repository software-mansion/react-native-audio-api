#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/decoding/SeekDecoderDaemon.h>
#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
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

inline constexpr auto FRAME_SPSC_OVERFLOW_STRATEGY =
    audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL;
inline constexpr auto FRAME_SPSC_WAIT_STRATEGY =
    audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT;
inline constexpr auto FRAME_SPSC_CHANNEL_CAPACITY = 64;

inline constexpr auto COMMAND_SPSC_OVERFLOW_STRATEGY =
    audioapi::channels::spsc::OverflowStrategy::OVERWRITE_ON_FULL;
inline constexpr auto COMMAND_SPSC_WAIT_STRATEGY =
    audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT;
inline constexpr auto COMMAND_SPSC_CHANNEL_CAPACITY = 16;

inline constexpr auto ON_POSITION_CHANGED_INTERVAL = 0.25f;

/// @brief Decodes a file or in-memory buffer and plays it as a scheduled source.
/// @note When routed through MediaElementAudioSourceNode, this node outputs silence and the media node pulls decoded audio.
/// @note Seek commands are executed from the JS thread and delegated to @ref SeekDecoderDaemon, which performs the seek and decoding on a worker thread,
// then sends decoded frames back to the audio thread via SPSC channels.
class AudioFileSourceNode : public AudioScheduledSourceNode {
  friend class MediaElementAudioSourceNode;

 public:
  explicit AudioFileSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioFileSourceOptions &options);
  ~AudioFileSourceNode() override = default;
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

  /// @brief Registers the JS callback id for position-changed events.
  /// @note Audio thread only.
  void setOnPositionChangedCallbackId(uint64_t callbackId);

  /// @brief Unregisters a position-changed handler.
  void unregisterOnPositionChangedCallback(uint64_t callbackId);

  /// @brief Enables looping at end-of-file.
  void setLoop(bool v) {
    loop_ = v;
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

  /// @brief Dispatches position-changed events at the configured interval.
  /// @note Audio thread only.
  void sendOnPositionChangedEvent(int framesPlayed);

  /// @brief Updates playback clock after a successful offloaded seek.
  void applyPlaybackStateAfterSuccessfulSeek(double seconds);

  /// @brief Reads decoded interleaved frames from the SPSC channel, deinterleaves them into @p destBuffer, and applies volume.
  [[nodiscard]] size_t readInterleavedFrames(
      const std::shared_ptr<DSPAudioBuffer> &destBuffer,
      size_t framesToRead);

  /// @brief Daemon thread for decoding and seeking
  std::unique_ptr<SeekDecoderDaemon> seekDecoderDaemon_;
  std::thread seekDecoderThread_;

  /// @brief Connects to the destination when leaving media routing while playback is active.
  /// @note Audio thread only.
  void ensureConnectedForDirectPlayback();

  uint64_t onPositionChangedCallbackId_ = 0;
  int onPositionChangedInterval_;
  int onPositionChangedTime_ = 0;

  /// @brief SPSC for JS -> Daemon thread communication (seek event)
  channels::spsc::Sender<SeekRequest, COMMAND_SPSC_OVERFLOW_STRATEGY, COMMAND_SPSC_WAIT_STRATEGY>
      commandSender_;

  /// @brief SPSC for Daemon thread -> Audio thread communication (decoded frames)
  channels::spsc::Receiver<DecoderData, FRAME_SPSC_OVERFLOW_STRATEGY, FRAME_SPSC_WAIT_STRATEGY>
      frameReceiver_;
};

} // namespace audioapi
