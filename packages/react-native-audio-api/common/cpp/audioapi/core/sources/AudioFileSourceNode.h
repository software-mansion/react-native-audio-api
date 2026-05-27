#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/miniaudio/MiniAudioDecoding.h>
#include <audioapi/utils/TaskOffloader.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace audioapi::channels;

namespace audioapi {

struct AudioFileSourceOptions;
class MediaElementAudioSourceNode;

/// @brief Target time for an offloaded seek task.
struct OffloadedSeekRequest {
  double seconds = 0;
  OffloadedSeekRequest() = default;
  explicit OffloadedSeekRequest(double t) : seconds(t) {}
};

/// @brief Decoder input and scratch buffer shared by the file source and media-element routing.
struct AudioFileDecoderState {
  std::vector<uint8_t> memoryData;
  std::string filePath;
  std::vector<float> interleavedBuffer;
  int channels = 0;
  float sampleRate = 0;
};

/// @brief Decodes a file or in-memory buffer and plays it as a scheduled source.
/// @note When routed through MediaElementAudioSourceNode, this node outputs silence and the media node pulls decoded audio.
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
  /// @brief Decodes and mixes samples for direct or media-element playback.
  /// @note Audio thread only.
  std::shared_ptr<DSPAudioBuffer> processDecodedOutput(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess);

  /// @brief Opens the decoder from file path or memory and prepares scratch buffers.
  void initDecoders(
      bool useFilePath,
      const std::shared_ptr<BaseAudioContext> &context,
      const std::shared_ptr<AudioFileDecoderState> &state);

  std::shared_ptr<AudioFileDecoderState> decoderState_;
  std::unique_ptr<decoding::IncrementalAudioDecoder> decoder_;

  float volume_;
  bool requiresFFmpeg_;
  bool filePaused_{false};
  bool loop_{false};
  double duration_{0};
  double sampleRate_{0};

  std::atomic<uint64_t> activeMediaBindingId_{0};
  std::atomic<double> currentTime_{0};

  static constexpr double ON_POSITION_CHANGED_INTERVAL = 0.25f;
  static constexpr int SEEK_OFFLOADER_WORKER_COUNT = 16;

  /// @brief Reads up to @p frameCount interleaved PCM frames into @p buf.
  /// @note Audio thread only.
  size_t readFrames(float *buf, size_t frameCount);

  /// @brief Fills the remainder of a region after EOF, seeking to start when looping.
  /// @note Audio thread only.
  size_t handleEof(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      size_t regionFrames,
      size_t framesRead,
      size_t destFrameOffset);

  /// @brief Dispatches position-changed events at the configured interval.
  /// @note Audio thread only.
  void sendOnPositionChangedEvent(int framesPlayed);

  /// @brief Updates playback clock after a successful offloaded seek.
  void applyPlaybackStateAfterSuccessfulSeek(double seconds);

  /// @brief Worker-thread seek implementation.
  void runOffloadedSeekTask(OffloadedSeekRequest req);

  /// @brief Connects to the destination when leaving media routing while playback is active.
  /// @note Audio thread only.
  void ensureConnectedForDirectPlayback();

  uint64_t onPositionChangedCallbackId_ = 0;
  int onPositionChangedInterval_;
  int onPositionChangedTime_ = 0;
  std::atomic<bool> onPositionChangedFlush_{true};

  /// Pending offloaded seeks; while > 0 the audio thread must not read the decoder (outputs silence).
  std::atomic<int> pendingOffloadedSeeks_{0};

  std::unique_ptr<task_offloader::TaskOffloader<
      OffloadedSeekRequest,
      spsc::OverflowStrategy::OVERWRITE_ON_FULL,
      spsc::WaitStrategy::ATOMIC_WAIT>>
      seekOffloader_;
};

} // namespace audioapi
