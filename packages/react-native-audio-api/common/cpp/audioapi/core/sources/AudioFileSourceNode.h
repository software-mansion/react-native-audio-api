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
#include <memory>
#include <string>
#include <vector>

using namespace audioapi::channels;

namespace audioapi {

struct AudioFileSourceOptions;

struct OffloadedSeekRequest {
  double seconds = 0;
  OffloadedSeekRequest() = default;
  explicit OffloadedSeekRequest(double t) : seconds(t) {}
};

struct AudioFileDecoderState {
  std::vector<uint8_t> memoryData;
  std::string filePath;
  std::vector<float> interleavedBuffer;
  int channels = 0;
  float sampleRate = 0;
};

class AudioFileSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioFileSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioFileSourceOptions &options);
  ~AudioFileSourceNode() override = default;

  void disable() override;

  void start(double when) override;

  void setVolume(float v) {
    volume_ = v;
  }

  void pause();

  /// @note Audio Thread only
  void setOnPositionChangedCallbackId(uint64_t callbackId);
  void unregisterOnPositionChangedCallback(uint64_t callbackId);

  void setLoop(bool v) {
    loop_ = v;
  }

  double getDuration() const {
    return duration_;
  }

  double getCurrentTime() const {
    return currentTime_.load(std::memory_order_acquire);
  }

  void seekToTime(double seconds);

 protected:
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  void initDecoders(
      bool useFilePath,
      const std::shared_ptr<BaseAudioContext> &context,
      const std::shared_ptr<AudioFileDecoderState> &state);

  std::shared_ptr<AudioFileDecoderState> decoderState_;
  std::unique_ptr<decoding::IIncrementalAudioDecoder> decoder_;
  float volume_;
  bool requiresFFmpeg_;
  bool filePaused_{false};
  bool loop_{false};
  double duration_{0};
  std::atomic<double> currentTime_{0};
  double sampleRate_{0};
  static constexpr double ON_POSITION_CHANGED_INTERVAL = 0.25f;
  static constexpr int SEEK_OFFLOADER_WORKER_COUNT = 16;

  size_t readFrames(float *buf, size_t frameCount);
  bool seekDecoderToTime(double seconds);
  void writeInterleavedToBufferAtOffset(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      const AudioFileDecoderState &state,
      size_t destFrameOffset,
      size_t frameCount) const;
  size_t handleEof(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      size_t regionFrames,
      size_t framesRead,
      size_t destFrameOffset);

  void sendOnPositionChangedEvent(int framesPlayed);

  void applyPlaybackStateAfterSuccessfulSeek(double seconds);
  void runOffloadedSeekTask(OffloadedSeekRequest req);

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
