/*
 * This file dynamically links to the FFmpeg library, which is licensed under
 * the GNU Lesser General Public License (LGPL) version 2.1 or later.
 *
 * Our own code in this file is licensed under the MIT License and dynamic
 * linking allows you to use this code without your entire project being subject
 * to the terms of the LGPL. However, note that if you link statically to
 * FFmpeg, you must comply with the terms of the LGPL for FFmpeg itself.
 */

#pragma once

#include <audioapi/decoding/IncrementalAudioDecoder.h>
#include <audioapi/utils/Macros.h>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace audioapi::ffmpeg_decoder {

/**
 * FFmpeg decoder for remote HTTP(S) / HLS (`openUrl`). Local file and memory
 * decode use OS / miniaudio instead — `openFile` / `openMemory` remain as
 * interface stubs (or local-file open for path-based demux when needed).
 *
 * Usage: openUrl → readPcmFrames repeatedly → close.
 */
class FFmpegDecoder : public decoding::IncrementalAudioDecoder {
 public:
  FFmpegDecoder() = default;
  ~FFmpegDecoder() override;
  DELETE_COPY_AND_MOVE(FFmpegDecoder);

  [[nodiscard]] decoding::DecoderResult openFile(int outputSampleRate, const std::string &path)
      override;

  [[nodiscard]] decoding::DecoderResult openUrl(
      int outputSampleRate,
      const std::string &url,
      const std::map<std::string, std::string> &headers = {}) override;

  /// Not used for batch decode (OS / miniaudio own memory). Returns an error.
  [[nodiscard]] decoding::DecoderResult
  openMemory(int outputSampleRate, const void *data, size_t size) override;

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

  [[nodiscard]] decoding::DecoderResult seekToTime(double seconds) override;

 private:
  [[nodiscard]] decoding::DecoderResult initializeDecodedStreams(
      int outputSampleRate,
      const char *errorContext);
  [[nodiscard]] decoding::DecoderResult setupSwr();
  [[nodiscard]] decoding::DecoderResult feedPipeline();
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

} // namespace audioapi::ffmpeg_decoder
