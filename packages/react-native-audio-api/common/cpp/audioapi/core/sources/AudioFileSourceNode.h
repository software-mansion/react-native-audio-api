#pragma once

#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/miniaudio/miniaudio.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

struct AudioFileSourceOptions;

struct AudioFileDecoderState {
  std::vector<uint8_t> memoryData;
  std::vector<float> interleavedBuffer;
  int channels = 0;
  float sampleRate = 0;
  std::string filePath;
};

class AudioFileSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioFileSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioFileSourceOptions &options);
  ~AudioFileSourceNode() override = default;

  /// @note Audio Thread only
  void setDecoderState(const std::shared_ptr<AudioFileDecoderState> &state);

  void disable() override;

  void start(double when) override;

  float getVolume() const {
    return volume_.load(std::memory_order_acquire);
  }

  void setVolume(float v) {
    volume_.store(v, std::memory_order_release);
  }

  void pause();

  bool getLoop() const {
    return loop_.load(std::memory_order_acquire);
  }

  void setLoop(bool v) {
    loop_.store(v, std::memory_order_release);
  }

  double getDuration() const {
    return duration_.load(std::memory_order_acquire);
  }

  double getCurrentTime() const {
    return currentTime_.load(std::memory_order_acquire);
  }

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
  bool FFmpegNeeded_;
#if !RN_AUDIO_API_FFMPEG_DISABLED
  ffmpegdecoder::FFmpegDecoder decoder;
  ffmpegdecoder::FFmpegDecoderConfig cfg;
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  std::atomic<bool> filePaused_{false};
  bool fileStarted_{false};
  std::atomic<bool> loop_{false};
  std::atomic<double> duration_{0};
  std::atomic<double> currentTime_{0};
  size_t totalFramesRead_{0};
  double sampleRate_{0};

  size_t readFrames(float *buf, size_t frameCount);
  bool seekToStart();
  static void writeInterleavedToBuffer(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      const AudioFileDecoderState &state,
      size_t destSampleOffset,
      size_t frameCount,
      float vol);
  size_t handleEof(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      size_t nonSilentFrames,
      size_t framesRead,
      float vol,
      size_t startOffset);
};

} // namespace audioapi
