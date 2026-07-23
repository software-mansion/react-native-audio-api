/*
 * This file dynamically links to the FFmpeg library, which is licensed under
 * the GNU Lesser General Public License (LGPL) version 2.1 or later.
 *
 * Our own code in this file is licensed under the MIT License and dynamic
 * linking allows you to use this code without your entire project being subject
 * to the terms of the LGPL. However, note that if you link statically to
 * FFmpeg, you must comply with the terms of the LGPL for FFmpeg itself.
 */

#include <audioapi/decoding/backends/FfmpegDecoder.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <thread>

#if !RN_AUDIO_API_FFMPEG_DISABLED
extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
#include <libavutil/rational.h>
}

namespace audioapi::decoding::ffmpeg {

namespace {

std::string parseFFmpegError(int errorCode) {
  std::array<char, AV_ERROR_MAX_STRING_SIZE> errorBuffer{};
  if (av_strerror(errorCode, errorBuffer.data(), errorBuffer.size()) == 0) {
    return std::string(errorBuffer.data()) + " (" + std::to_string(errorCode) + ")";
  }
  return "Unknown FFmpeg error (" + std::to_string(errorCode) + ")";
}

inline constexpr auto HLS_READ_RETRY_DELAY = std::chrono::milliseconds(10);
inline constexpr int HLS_MAX_READ_RETRIES = 100;

bool isTransientReadError(int errorCode) {
  return errorCode == AVERROR(EIO) || errorCode == AVERROR(ECONNRESET) ||
      errorCode == AVERROR(ETIMEDOUT) || errorCode == AVERROR(EPIPE);
}

std::string buildFfmpegHttpHeaders(const std::map<std::string, std::string> &headers) {
  std::string headerBlock;
  headerBlock.reserve(headers.size() * 32);
  for (const auto &[key, value] : headers) {
    if (key.empty()) {
      continue;
    }
    headerBlock += key;
    headerBlock += ": ";
    headerBlock += value;
    headerBlock += "\r\n";
  }
  return headerBlock;
}

} // namespace

int findAudioStreamIndex(AVFormatContext *fmt_ctx) {
  for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

decoding::DecoderResult
openCodec(AVFormatContext *fmt_ctx, int &audio_stream_index, AVCodecContext **out_codec) {
  audio_stream_index = findAudioStreamIndex(fmt_ctx);
  if (audio_stream_index < 0) {
    return Err("FfmpegDecoder::openCodec failed: no audio stream found");
  }
  AVCodecParameters *codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;
  const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
  if (codec == nullptr) {
    return Err("FfmpegDecoder::openCodec failed: decoder not found");
  }
  AVCodecContext *ctx = avcodec_alloc_context3(codec);
  if (ctx == nullptr) {
    return Err("FfmpegDecoder::openCodec failed: avcodec_alloc_context3 returned null");
  }
  const int parametersResult = avcodec_parameters_to_context(ctx, codecpar);
  if (parametersResult < 0) {
    avcodec_free_context(&ctx);
    return Err(
        "FfmpegDecoder::openCodec failed: avcodec_parameters_to_context failed: " +
        parseFFmpegError(parametersResult));
  }
  const int openResult = avcodec_open2(ctx, codec, nullptr);
  if (openResult < 0) {
    avcodec_free_context(&ctx);
    return Err(
        "FfmpegDecoder::openCodec failed: avcodec_open2 failed: " + parseFFmpegError(openResult));
  }
  *out_codec = ctx;
  return Ok(None);
}

FfmpegDecoder::~FfmpegDecoder() {
  close();
}

void FfmpegDecoder::close() {
  if (resampled_data_ != nullptr) {
    av_freep(&resampled_data_[0]);
    av_freep(&resampled_data_);
  }
  max_resampled_samples_ = 0;
  if (swr_ != nullptr) {
    swr_free(&swr_);
  }
  if (packet_ != nullptr) {
    av_packet_free(&packet_);
  }
  if (frame_ != nullptr) {
    av_frame_free(&frame_);
  }
  if (codec_ctx_ != nullptr) {
    avcodec_free_context(&codec_ctx_);
  }
  if (fmt_ctx_ != nullptr) {
    avformat_close_input(&fmt_ctx_);
  }
  leftover_.clear();
  leftover_offset_ = 0;
  audio_stream_index_ = -1;
  resetOpenMetadata();
  is_hls_streaming_ = false;
}

void FfmpegDecoder::detectHlsStreamingMode() {
  is_hls_streaming_ = fmt_ctx_ != nullptr && fmt_ctx_->iformat != nullptr &&
      fmt_ctx_->iformat->name != nullptr && std::strcmp(fmt_ctx_->iformat->name, "hls") == 0;
}

decoding::DecoderResult FfmpegDecoder::setupSwr() {
  swr_ = swr_alloc();
  if (swr_ == nullptr) {
    return Err("FfmpegDecoder::setupSwr failed: swr_alloc returned null");
  }
  av_opt_set_chlayout(swr_, "in_chlayout", &codec_ctx_->ch_layout, 0);
  av_opt_set_int(swr_, "in_sample_rate", codec_ctx_->sample_rate, 0);
  av_opt_set_sample_fmt(swr_, "in_sample_fmt", codec_ctx_->sample_fmt, 0);

  AVChannelLayout out_layout;
  av_channel_layout_default(&out_layout, outputChannels_);
  av_opt_set_chlayout(swr_, "out_chlayout", &out_layout, 0);
  av_opt_set_int(swr_, "out_sample_rate", outputSampleRate_, 0);
  av_opt_set_sample_fmt(swr_, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
  const int swrInitResult = swr_init(swr_);
  if (swrInitResult < 0) {
    av_channel_layout_uninit(&out_layout);
    return Err(
        "FfmpegDecoder::setupSwr failed: swr_init failed: " + parseFFmpegError(swrInitResult));
  }
  av_channel_layout_uninit(&out_layout);

  const int allocResult = av_samples_alloc_array_and_samples(
      &resampled_data_,
      nullptr,
      outputChannels_,
      decoding::AudioDecoderBackend::CHUNK_SIZE,
      AV_SAMPLE_FMT_FLT,
      0);
  if (allocResult < 0) {
    return Err(
        "FfmpegDecoder::setupSwr failed: av_samples_alloc_array_and_samples failed: " +
        parseFFmpegError(allocResult));
  }
  max_resampled_samples_ = static_cast<int>(decoding::AudioDecoderBackend::CHUNK_SIZE);
  return Ok(None);
}

decoding::DecoderResult FfmpegDecoder::initializeDecodedStreams(
    int outputSampleRate,
    const char *errorContext) {
  const int streamInfoResult = avformat_find_stream_info(fmt_ctx_, nullptr);
  if (streamInfoResult < 0) {
    avformat_close_input(&fmt_ctx_);
    fmt_ctx_ = nullptr;
    return Err(
        std::string(errorContext) +
        " failed: avformat_find_stream_info failed: " + parseFFmpegError(streamInfoResult));
  }
  detectHlsStreamingMode();
  auto codecResult = openCodec(fmt_ctx_, audio_stream_index_, &codec_ctx_);
  if (codecResult.is_err()) {
    avformat_close_input(&fmt_ctx_);
    fmt_ctx_ = nullptr;
    return codecResult;
  }
  outputChannels_ = codec_ctx_->ch_layout.nb_channels;
  outputSampleRate_ = (outputSampleRate > 0) ? outputSampleRate : codec_ctx_->sample_rate;

  packet_ = av_packet_alloc();
  frame_ = av_frame_alloc();
  if (packet_ == nullptr) {
    close();
    return Err(std::string(errorContext) + " failed: av_packet_alloc returned null");
  }
  if (frame_ == nullptr) {
    close();
    return Err(std::string(errorContext) + " failed: av_frame_alloc returned null");
  }
  auto swrResult = setupSwr();
  if (swrResult.is_err()) {
    close();
    return swrResult;
  }
  framePosition_ = 0;
  cacheDurationFromContainer();
  return Ok(None);
}

void FfmpegDecoder::cacheDurationFromContainer() {
  if (fmt_ctx_ == nullptr || audio_stream_index_ < 0) {
    totalPcmFrames_ = 0;
    return;
  }
  AVStream *st = fmt_ctx_->streams[audio_stream_index_];
  if (st == nullptr) {
    totalPcmFrames_ = 0;
    return;
  }

  auto validSeconds = [](double s) -> bool {
    return s > 0 && std::isfinite(s);
  };

  // Prefer per-stream duration (e.g. MP4 mdhd) — often exact vs container-level
  // guesses that trigger AAC “bitrate duration” warnings.
  if (st->duration != AV_NOPTS_VALUE && st->duration > 0) {
    double t = static_cast<double>(st->duration) * av_q2d(st->time_base);
    if (validSeconds(t)) {
      setTotalPcmFramesFromDuration(t);
      return;
    }
  }

  if (fmt_ctx_->duration != AV_NOPTS_VALUE && fmt_ctx_->duration >= 0) {
    double t = static_cast<double>(fmt_ctx_->duration) / static_cast<double>(AV_TIME_BASE);
    if (validSeconds(t)) {
      setTotalPcmFramesFromDuration(t);
      return;
    }
  }

  totalPcmFrames_ = 0;
}

decoding::DecoderResult FfmpegDecoder::open(const decoding::RemoteUrlSource &source) {
  close();
  if (source.url.empty()) {
    return Err("FfmpegDecoder::open failed: url is empty");
  }

  AVDictionary *options = nullptr;
  av_dict_set(&options, "protocol_whitelist", "file,http,https,tcp,tls,crypto,hls", 0);

  const std::string headerBlock = buildFfmpegHttpHeaders(source.httpHeaders);
  if (!headerBlock.empty()) {
    av_dict_set(&options, "headers", headerBlock.c_str(), 0);
  }

  const int openInputResult = avformat_open_input(&fmt_ctx_, source.url.c_str(), nullptr, &options);
  av_dict_free(&options);
  if (openInputResult < 0) {
    fmt_ctx_ = nullptr;
    return Err(
        "FfmpegDecoder::open failed: avformat_open_input failed: " +
        parseFFmpegError(openInputResult));
  }
  return initializeDecodedStreams(source.sampleRate, "FfmpegDecoder::open");
}

void FfmpegDecoder::appendFrameResampled(AVFrame *frame) {
  int out_samples = swr_get_out_samples(swr_, frame->nb_samples);
  if (out_samples > max_resampled_samples_) {
    av_freep(&resampled_data_[0]);
    av_freep(&resampled_data_);
    max_resampled_samples_ = out_samples;
    if (av_samples_alloc_array_and_samples(
            &resampled_data_,
            nullptr,
            outputChannels_,
            max_resampled_samples_,
            AV_SAMPLE_FMT_FLT,
            0) < 0) {
      return;
    }
  }
  int converted = swr_convert(
      swr_,
      resampled_data_,
      max_resampled_samples_,
      const_cast<const uint8_t **>(frame->data),
      frame->nb_samples);
  if (converted > 0) {
    size_t n = static_cast<size_t>(converted) * static_cast<size_t>(outputChannels_);
    const float *src = reinterpret_cast<float *>(resampled_data_[0]);
    leftover_.insert(leftover_.end(), src, src + n);
  }
}

decoding::DecoderResult FfmpegDecoder::feedPipeline() {
  int readRetries = 0;

  for (;;) {
    int r = avcodec_receive_frame(codec_ctx_, frame_);
    if (r == 0) {
      readRetries = 0;
      appendFrameResampled(frame_);
      return Ok(None);
    }
    if (r == AVERROR_EOF) {
      if (!leftover_.empty()) {
        return Ok(None);
      }
      if (!is_hls_streaming_) {
        return Err("FfmpegDecoder::feedPipeline reached end of stream");
      }
      // HLS live: need more segment data — fall through to av_read_frame.
    } else if (r != AVERROR(EAGAIN)) {
      return Err(
          "FfmpegDecoder::feedPipeline failed: avcodec_receive_frame failed: " +
          parseFFmpegError(r));
    }

    r = av_read_frame(fmt_ctx_, packet_);
    if (r == AVERROR_EOF) {
      if (is_hls_streaming_) {
        std::this_thread::sleep_for(HLS_READ_RETRY_DELAY);
        continue;
      }
      const int flushResult = avcodec_send_packet(codec_ctx_, nullptr);
      if (flushResult < 0) {
        return Err(
            "FfmpegDecoder::feedPipeline failed: avcodec_send_packet flush failed: " +
            parseFFmpegError(flushResult));
      }
      continue;
    }
    if (r < 0) {
      if (is_hls_streaming_ && isTransientReadError(r) && readRetries < HLS_MAX_READ_RETRIES) {
        readRetries++;
        std::this_thread::sleep_for(HLS_READ_RETRY_DELAY);
        continue;
      }
      return Err(
          "FfmpegDecoder::feedPipeline failed: av_read_frame failed: " + parseFFmpegError(r));
    }
    readRetries = 0;
    if (packet_->stream_index != audio_stream_index_) {
      av_packet_unref(packet_);
      continue;
    }
    r = avcodec_send_packet(codec_ctx_, packet_);
    av_packet_unref(packet_);
    if (r < 0) {
      return Err(
          "FfmpegDecoder::feedPipeline failed: avcodec_send_packet failed: " + parseFFmpegError(r));
    }
  }
}

// todo: offload this call to a separate thread because seeking decoder can take a while
// current implementation suspends audio thread, which disable multiple playbacks
decoding::DecoderResult FfmpegDecoder::seekToTime(double seconds) {
  if (!isOpen() || audio_stream_index_ < 0 || outputSampleRate_ <= 0) {
    return Err("FfmpegDecoder::seekToTime failed: decoder is not open");
  }
  float dur = getDurationInSeconds();
  if (dur > 0 && std::isfinite(dur)) {
    seconds = std::clamp(seconds, 0.0, static_cast<double>(dur));
  } else {
    seconds = std::max(0.0, seconds);
    if (!std::isfinite(seconds)) {
      return Err("FfmpegDecoder::seekToTime failed: seconds is not finite");
    }
  }

  auto ts = static_cast<int64_t>(seconds * static_cast<double>(AV_TIME_BASE));
  const int seekResult = avformat_seek_file(fmt_ctx_, -1, INT64_MIN, ts, INT64_MAX, 0);
  if (seekResult < 0) {
    return Err(
        "FfmpegDecoder::seekToTime failed: avformat_seek_file failed: " +
        parseFFmpegError(seekResult));
  }
  avcodec_flush_buffers(codec_ctx_);
  leftover_.clear();
  leftover_offset_ = 0;
  framePosition_ =
      static_cast<int64_t>(std::llround(seconds * static_cast<double>(outputSampleRate_)));
  return Ok(None);
}

size_t FfmpegDecoder::readPcmFrames(float *outInterleaved, size_t frameCount) {
  if (!isOpen() || outInterleaved == nullptr || frameCount == 0 || outputChannels_ <= 0) {
    return 0;
  }
  size_t delivered = 0;
  const auto ch = static_cast<size_t>(outputChannels_);

  while (delivered < frameCount) {
    size_t need = frameCount - delivered;
    size_t available_samples =
        leftover_.size() > leftover_offset_ ? leftover_.size() - leftover_offset_ : 0;
    size_t leftover_frames = available_samples / ch;
    if (leftover_frames > 0) {
      size_t take = std::min(need, leftover_frames);
      size_t samples = take * ch;
      memcpy(
          outInterleaved + delivered * ch,
          leftover_.data() + leftover_offset_,
          samples * sizeof(float));
      leftover_offset_ += samples;
      if (leftover_offset_ >= leftover_.size()) {
        leftover_.clear();
        leftover_offset_ = 0;
      }
      delivered += take;
    } else if (feedPipeline().is_err()) {
      break;
    }
  }
  framePosition_ += static_cast<int64_t>(delivered);
  return delivered;
}

} // namespace audioapi::decoding::ffmpeg

#else

namespace audioapi::decoding::ffmpeg {
FfmpegDecoder::~FfmpegDecoder() = default;
void FfmpegDecoder::close() {}
decoding::DecoderResult FfmpegDecoder::open(const decoding::RemoteUrlSource &) {
  return Err("FFmpeg is disabled");
}
decoding::DecoderResult FfmpegDecoder::seekToTime(double) {
  return Err("FFmpeg is disabled");
}
size_t FfmpegDecoder::readPcmFrames(float *, size_t) {
  return 0;
}

} // namespace audioapi::decoding::ffmpeg

#endif // !RN_AUDIO_API_FFMPEG_DISABLED
