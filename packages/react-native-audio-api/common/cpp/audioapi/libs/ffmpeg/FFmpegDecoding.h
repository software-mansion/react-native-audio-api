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

#include <audioapi/utils/AudioBuffer.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

class AudioBuffer;

namespace audioapi::ffmpegdecoder {

/// Opaque IO state for openMemory (must outlive decode until close).
struct MemoryIOContext {
  const uint8_t *data = nullptr;
  size_t size = 0;
  size_t pos = 0;
};

/// Step 1 — like ma_decoder_config_init: desired output sample rate (0 = use stream rate).
struct FFmpegDecoderConfig {
  int outputSampleRate = 0;
};

/// Initialize decoder config (mirrors miniaudio-style config step).
inline void ffmpegDecoderConfigInit(FFmpegDecoderConfig *cfg, int outputSampleRate) {
  if (cfg != nullptr) {
    cfg->outputSampleRate = outputSampleRate;
  }
}

/**
 * FFmpeg decoder with incremental read, analogous to ma_decoder:
 *   1) ffmpegDecoderConfigInit
 *   2) openFile or openMemory
 *   3) readPcmFrames repeatedly; 0 returned = end of stream
 *   4) close when done
 *
 * For openMemory, \p data must remain valid until close().
 */
class FFmpegDecoder {
 public:
  FFmpegDecoder() = default;
  FFmpegDecoder(const FFmpegDecoder &) = delete;
  FFmpegDecoder &operator=(const FFmpegDecoder &) = delete;
  FFmpegDecoder(FFmpegDecoder &&other) = delete;
  FFmpegDecoder &operator=(FFmpegDecoder &&other) = delete;
  ~FFmpegDecoder();

  /// @brief Opens a file for decoding.
  /// @param cfg The configuration for the decoder.
  /// @param path The path to the file.
  /// @return True if the file was opened successfully, false otherwise.
  [[nodiscard]] bool openFile(const FFmpegDecoderConfig &cfg, const std::string &path);

  /// @brief Opens a memory block for decoding.
  /// @param cfg The configuration for the decoder.
  /// @param data The data to decode.
  /// @param size The size of the data.
  /// @return True if the memory block was opened successfully, false otherwise.
  [[nodiscard]] bool openMemory(const FFmpegDecoderConfig &cfg, const void *data, size_t size);

  /// @brief Reads frames from the decoder.
  /// @param outInterleaved The output buffer for the frames.
  /// @param frameCount The maximum number of frames to read.
  /// @return The number of frames actually read (0 = EOF).
  [[nodiscard]] size_t readPcmFrames(float *outInterleaved, size_t frameCount);

  /// @brief Closes the decoder.
  void close();

  /// @brief Checks if the decoder is open.
  /// @return True if the decoder is open, false otherwise.
  [[nodiscard]] bool isOpen() const { return fmt_ctx_ != nullptr && codec_ctx_ != nullptr; }
  [[nodiscard]] int outputChannels() const { return output_channels_; }
  [[nodiscard]] int outputSampleRate() const { return output_sample_rate_; }

  /// @brief Duration in seconds. Returns 0 if unknown.
  [[nodiscard]] float getDurationInSeconds() const;

  /// @brief Current playback position in seconds (frames read / sample rate).
  [[nodiscard]] float getCurrentPositionInSeconds() const;

  /// @brief Seeks to a playback position in seconds (output / resampled timeline).
  /// @return True if seek succeeded.
  [[nodiscard]] bool seekToTime(double seconds);

  static constexpr size_t CHUNK_SIZE = 4096;

 private:
  bool setupSwr();
  bool feedPipeline();
  void appendFrameResampled(AVFrame *frame);

  AVFormatContext *fmt_ctx_ = nullptr;
  AVCodecContext *codec_ctx_ = nullptr;
  SwrContext *swr_ = nullptr;
  AVPacket *packet_ = nullptr;
  AVFrame *frame_ = nullptr;

  uint8_t **resampled_data_ = nullptr;
  int max_resampled_samples_ = 0;

  std::unique_ptr<MemoryIOContext> mem_io_;
  AVIOContext *avio_ctx_ = nullptr;

  std::vector<float> leftover_;
  size_t leftover_offset_ = 0;
  int audio_stream_index_ = -1;
  int output_channels_ = 0;
  int output_sample_rate_ = 0;
  size_t total_output_frames_ = 0;
};

// --- One-shot decode (existing API) ----------------------------------------

std::shared_ptr<AudioBuffer> decodeWithMemoryBlock(const void *data, size_t size, int sample_rate);
std::shared_ptr<AudioBuffer> decodeWithFilePath(const std::string &path, int sample_rate);

} // namespace audioapi::ffmpegdecoder
