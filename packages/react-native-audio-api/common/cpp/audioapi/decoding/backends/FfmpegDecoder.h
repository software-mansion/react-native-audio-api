#pragma once

#include <audioapi/decoding/DecoderSource.h>
#include <audioapi/decoding/backends/AudioDecoderBackend.h>
#include <audioapi/utils/Macros.h>
#include <cstddef>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace audioapi::decoding::ffmpeg {

/**
 * FFmpeg decoder for remote HTTP(S) / HLS and local HLS playlists.
 *
 * Usage: open via DecoderFactory → readPcmFrames repeatedly → close.
 */
class FfmpegDecoder : public AudioDecoderBackend {
 public:
  FfmpegDecoder() = default;
  ~FfmpegDecoder() override;
  DELETE_COPY_AND_MOVE(FfmpegDecoder);

  [[nodiscard]] DecoderResult open(const LocalFileSource &source);
  [[nodiscard]] DecoderResult open(const RemoteUrlSource &source);

  [[nodiscard]] size_t readPcmFrames(float *outInterleaved, size_t frameCount) override;

  void close() override;

  [[nodiscard]] bool isOpen() const override {
    return fmt_ctx_ != nullptr && codec_ctx_ != nullptr;
  }
  [[nodiscard]] int outputChannels() const override {
    return output_channels_;
  }
  [[nodiscard]] int outputSampleRate() const override {
    return output_sample_rate_;
  }

  [[nodiscard]] bool isHlsStreaming() const override {
    return is_hls_streaming_;
  }

  [[nodiscard]] float getDurationInSeconds() const override;

  [[nodiscard]] float getCurrentPositionInSeconds() const override;

  [[nodiscard]] DecoderResult seekToTime(double seconds) override;
  [[nodiscard]] size_t getTotalPcmFrameCount() const override;

 private:
  [[nodiscard]] DecoderResult initializeDecodedStreams(
      int outputSampleRate,
      const char *errorContext);

  [[nodiscard]] DecoderResult setupSwr();
  [[nodiscard]] DecoderResult feedPipeline();
  void appendFrameResampled(AVFrame *frame);
  void detectHlsStreamingMode();

  bool is_hls_streaming_ = false;

  AVFormatContext *fmt_ctx_ = nullptr;
  AVCodecContext *codec_ctx_ = nullptr;
  SwrContext *swr_ = nullptr;
  AVPacket *packet_ = nullptr;
  AVFrame *frame_ = nullptr;

  uint8_t **resampled_data_ = nullptr;
  int max_resampled_samples_ = 0;

  std::vector<float> leftover_;
  size_t leftover_offset_ = 0;
  int audio_stream_index_ = -1;
  int output_channels_ = 0;
  int output_sample_rate_ = 0;
  size_t total_output_frames_ = 0;
};

} // namespace audioapi::decoding::ffmpeg
