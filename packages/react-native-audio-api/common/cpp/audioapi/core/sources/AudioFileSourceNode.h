#pragma once

#include <audioapi/core/AudioNode.h>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/miniaudio/miniaudio.h>
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

class AudioFileSourceNode : public AudioNode {
 public:
  explicit AudioFileSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioFileSourceOptions &options);
  ~AudioFileSourceNode() override = default;

  void disable() override;

  void start();

  float getVolume() const {
    return volume_.load(std::memory_order_acquire);
  }

  void setVolume(float v) {
    volume_.store(v, std::memory_order_release);
  }

  void pause();

  /// @note Audio Thread only
  void setOnPositionChangedCallbackId(uint64_t callbackId);
  void unregisterOnPositionChangedCallback(uint64_t callbackId);

  /// @note Audio Thread only
  void setOnEndedCallbackId(uint64_t callbackId);
  void unregisterOnEndedCallback(uint64_t callbackId);

  bool getLoop() const {
    return loop_.load(std::memory_order_acquire);
  }

  void setLoop(bool v) {
    loop_.store(v, std::memory_order_release);
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
  std::unique_ptr<ma_decoder> maDecoder_;
  std::atomic<float> volume_;
  bool requiresFFmpeg_;
#if !RN_AUDIO_API_FFMPEG_DISABLED
  ffmpegdecoder::FFmpegDecoder ffmpegDecoder_;
  ffmpegdecoder::FFmpegDecoderConfig cfg;
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  std::atomic<bool> filePaused_{false};
  std::atomic<bool> loop_{false};
  double duration_{0};
  std::atomic<double> currentTime_{0};
  double sampleRate_{0};
  const std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  static constexpr double ON_POSITION_CHANGED_INTERVAL = 0.25f;
  static constexpr int SEEK_OFFLOADER_WORKER_COUNT = 16;

  size_t readFrames(float *buf, size_t frameCount);
  bool seekDecoderToTime(double seconds);
  static void writeInterleavedToBuffer(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      const AudioFileDecoderState &state,
      size_t destSampleOffset,
      size_t frameCount,
      float vol);
  size_t handleEof(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      size_t framesToProcess,
      size_t framesRead,
      float vol);

  void sendOnPositionChangedEvent(int samplesWritten);
  void sendOnEndedEvent();

  void applyPlaybackStateAfterSuccessfulSeek(double seconds);
  void runOffloadedSeekTask(OffloadedSeekRequest req);

  uint64_t onPositionChangedCallbackId_ = 0;
  uint64_t onEndedCallbackId_ = 0;
  std::atomic<bool> playbackFinished_{false};
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
