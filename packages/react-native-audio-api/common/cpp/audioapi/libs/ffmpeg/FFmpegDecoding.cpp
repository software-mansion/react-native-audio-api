/*
 * This file dynamically links to the FFmpeg library, which is licensed under
 * the GNU Lesser General Public License (LGPL) version 2.1 or later.
 *
 * Our own code in this file is licensed under the MIT License and dynamic
 * linking allows you to use this code without your entire project being subject
 * to the terms of the LGPL. However, note that if you link statically to
 * FFmpeg, you must comply with the terms of the LGPL for FFmpeg itself.
 */

#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#if !RN_AUDIO_API_FFMPEG_DISABLED

#include <algorithm>
#include <cmath>
#include <cstring>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mathematics.h>
#include <libavutil/rational.h>
}

namespace audioapi::ffmpegdecoder {

int read_packet(void *opaque, uint8_t *buf, int buf_size) {
  auto *ctx = static_cast<MemoryIOContext *>(opaque);
  if (ctx->pos >= ctx->size) {
    return AVERROR_EOF;
  }
  int n = std::min(buf_size, static_cast<int>(ctx->size - ctx->pos));
  memcpy(buf, ctx->data + ctx->pos, n);
  ctx->pos += static_cast<size_t>(n);
  return n;
}

int64_t seek_packet(void *opaque, int64_t offset, int whence) {
  auto *ctx = static_cast<MemoryIOContext *>(opaque);
  switch (whence) {
    case SEEK_SET:
      ctx->pos = static_cast<size_t>(offset);
      break;
    case SEEK_CUR:
      ctx->pos += static_cast<size_t>(offset);
      break;
    case SEEK_END:
      ctx->pos = ctx->size + static_cast<size_t>(offset);
      break;
    case AVSEEK_SIZE:
      return static_cast<int64_t>(ctx->size);
    default:
      return AVERROR(EINVAL);
  }
  ctx->pos = std::min(ctx->pos, ctx->size);
  return static_cast<int64_t>(ctx->pos);
}

int findAudioStreamIndex(AVFormatContext *fmt_ctx) {
  for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool openCodec(AVFormatContext *fmt_ctx, int &audio_stream_index, AVCodecContext **out_codec) {
  audio_stream_index = findAudioStreamIndex(fmt_ctx);
  if (audio_stream_index < 0) {
    return false;
  }
  AVCodecParameters *codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;
  const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
  if (codec == nullptr) {
    return false;
  }
  AVCodecContext *ctx = avcodec_alloc_context3(codec);
  if (ctx == nullptr) {
    return false;
  }
  if (avcodec_parameters_to_context(ctx, codecpar) < 0) {
    avcodec_free_context(&ctx);
    return false;
  }
  if (avcodec_open2(ctx, codec, nullptr) < 0) {
    avcodec_free_context(&ctx);
    return false;
  }
  *out_codec = ctx;
  return true;
}

FFmpegDecoder::~FFmpegDecoder() {
  close();
}

void FFmpegDecoder::close() {
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
  if (avio_ctx_ != nullptr) {
    avio_context_free(&avio_ctx_);
  }
  mem_io_.reset();
  leftover_.clear();
  leftover_offset_ = 0;
  audio_stream_index_ = -1;
  output_channels_ = 0;
  output_sample_rate_ = 0;
  total_output_frames_ = 0;
}

bool FFmpegDecoder::setupSwr() {
  swr_ = swr_alloc();
  if (swr_ == nullptr) {
    return false;
  }
  av_opt_set_chlayout(swr_, "in_chlayout", &codec_ctx_->ch_layout, 0);
  av_opt_set_int(swr_, "in_sample_rate", codec_ctx_->sample_rate, 0);
  av_opt_set_sample_fmt(swr_, "in_sample_fmt", codec_ctx_->sample_fmt, 0);

  AVChannelLayout out_layout;
  av_channel_layout_default(&out_layout, output_channels_);
  av_opt_set_chlayout(swr_, "out_chlayout", &out_layout, 0);
  av_opt_set_int(swr_, "out_sample_rate", output_sample_rate_, 0);
  av_opt_set_sample_fmt(swr_, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
  if (swr_init(swr_) < 0) {
    av_channel_layout_uninit(&out_layout);
    return false;
  }
  av_channel_layout_uninit(&out_layout);

    if (av_samples_alloc_array_and_samples(
          &resampled_data_,
          nullptr,
          output_channels_,
          decoding::IIncrementalAudioDecoder::CHUNK_SIZE,
          AV_SAMPLE_FMT_FLT,
          0) < 0) {
    return false;
  }
  max_resampled_samples_ = static_cast<int>(decoding::IIncrementalAudioDecoder::CHUNK_SIZE);
  return true;
}

bool FFmpegDecoder::openFile(int outputSampleRate, const std::string &path) {
  close();
  if (path.empty()) {
    return false;
  }
  if (avformat_open_input(&fmt_ctx_, path.c_str(), nullptr, nullptr) < 0) {
    fmt_ctx_ = nullptr;
    return false;
  }
  if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
    avformat_close_input(&fmt_ctx_);
    fmt_ctx_ = nullptr;
    return false;
  }
  if (!openCodec(fmt_ctx_, audio_stream_index_, &codec_ctx_)) {
    avformat_close_input(&fmt_ctx_);
    fmt_ctx_ = nullptr;
    return false;
  }
  output_channels_ = codec_ctx_->ch_layout.nb_channels;
  output_sample_rate_ =
      (outputSampleRate > 0) ? outputSampleRate : codec_ctx_->sample_rate;

  packet_ = av_packet_alloc();
  frame_ = av_frame_alloc();
  if (packet_ == nullptr || frame_ == nullptr || !setupSwr()) {
    close();
    return false;
  }
  total_output_frames_ = 0;
  return true;
}

bool FFmpegDecoder::openMemory(
    int outputSampleRate,
    const void *data,
    size_t size) {
  close();
  if (data == nullptr || size == 0) {
    return false;
  }
  mem_io_ = std::make_unique<MemoryIOContext>();
  mem_io_->data = static_cast<const uint8_t *>(data);
  mem_io_->size = size;
  mem_io_->pos = 0;

  auto* io_buf =
      static_cast<uint8_t *>(av_malloc(decoding::IIncrementalAudioDecoder::CHUNK_SIZE));
  if (io_buf == nullptr) {
    close();
    return false;
  }
  avio_ctx_ = avio_alloc_context(
    io_buf,
    static_cast<int>(decoding::IIncrementalAudioDecoder::CHUNK_SIZE),
    0,
    mem_io_.get(),
    read_packet,
    nullptr,
    seek_packet);
  if (avio_ctx_ == nullptr) {
    av_free(io_buf);
    mem_io_.reset();
    return false;
  }

  fmt_ctx_ = avformat_alloc_context();
  if (fmt_ctx_ == nullptr) {
    close();
    return false;
  }
  fmt_ctx_->pb = avio_ctx_;

  if (avformat_open_input(&fmt_ctx_, nullptr, nullptr, nullptr) < 0) {
    close();
    return false;
  }
  if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
    close();
    return false;
  }
  if (!openCodec(fmt_ctx_, audio_stream_index_, &codec_ctx_)) {
    close();
    return false;
  }
  output_channels_ = codec_ctx_->ch_layout.nb_channels;
  output_sample_rate_ =
      (outputSampleRate > 0) ? outputSampleRate : codec_ctx_->sample_rate;

  packet_ = av_packet_alloc();
  frame_ = av_frame_alloc();
  if (packet_ == nullptr || frame_ == nullptr || !setupSwr()) {
    close();
    return false;
  }
  total_output_frames_ = 0;
  return true;
}

void FFmpegDecoder::appendFrameResampled(AVFrame *frame) {
  int out_samples = swr_get_out_samples(swr_, frame->nb_samples);
  if (out_samples > max_resampled_samples_) {
    av_freep(&resampled_data_[0]);
    av_freep(&resampled_data_);
    max_resampled_samples_ = out_samples;
    if (av_samples_alloc_array_and_samples(
            &resampled_data_,
            nullptr,
            output_channels_,
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
    size_t n = static_cast<size_t>(converted) * static_cast<size_t>(output_channels_);
    const float *src = reinterpret_cast<float *>(resampled_data_[0]);
    leftover_.insert(leftover_.end(), src, src + n);
  }
}

bool FFmpegDecoder::feedPipeline() {
  for (;;) {
    int r = avcodec_receive_frame(codec_ctx_, frame_);
    if (r == 0) {
      appendFrameResampled(frame_);
      return true;
    }
    if (r == AVERROR_EOF) {
      return !leftover_.empty();
    }
    if (r != AVERROR(EAGAIN)) {
      return false;
    }

    r = av_read_frame(fmt_ctx_, packet_);
    if (r == AVERROR_EOF) {
      if (avcodec_send_packet(codec_ctx_, nullptr) < 0) {
        return false;
      }
      continue;
    }
    if (r < 0) {
      return false;
    }
    if (packet_->stream_index != audio_stream_index_) {
      av_packet_unref(packet_);
      continue;
    }
    r = avcodec_send_packet(codec_ctx_, packet_);
    av_packet_unref(packet_);
    if (r < 0) {
      return false;
    }
  }
}

float FFmpegDecoder::getDurationInSeconds() const {
  if (!isOpen() || fmt_ctx_ == nullptr || audio_stream_index_ < 0) {
    return 0;
  }
  AVStream *st = fmt_ctx_->streams[audio_stream_index_];
  if (st == nullptr) {
    return 0;
  }

  auto validSeconds = [](double s) -> bool { return s > 0 && std::isfinite(s); };

  // Prefer per-stream duration (e.g. MP4 mdhd) — often exact vs container-level
  // guesses that trigger AAC “bitrate duration” warnings.
  if (st->duration != AV_NOPTS_VALUE && st->duration > 0) {
    double t = static_cast<double>(st->duration) * av_q2d(st->time_base);
    if (validSeconds(t)) {
      return static_cast<float>(t);
    }
  }

  if (fmt_ctx_->duration != AV_NOPTS_VALUE && fmt_ctx_->duration >= 0) {
    double t = static_cast<double>(fmt_ctx_->duration) / static_cast<double>(AV_TIME_BASE);
    if (validSeconds(t)) {
      return static_cast<float>(t);
    }
  }

  return 0;
}

float FFmpegDecoder::getCurrentPositionInSeconds() const {
  if (!isOpen() || output_sample_rate_ <= 0) {
    return 0;
  }
  return static_cast<float>(total_output_frames_) / static_cast<float>(output_sample_rate_);
}

// todo: offload this call to a separate thread because seeking decoder can take a while
// current implementation suspends audio thread, which disable multiple playbacks
bool FFmpegDecoder::seekToTime(double seconds) {
  if (!isOpen() || audio_stream_index_ < 0 || output_sample_rate_ <= 0) {
    return false;
  }
  float dur = getDurationInSeconds();
  if (dur > 0 && std::isfinite(dur)) {
    seconds = std::clamp(seconds, 0.0, static_cast<double>(dur));
  } else {
    seconds = std::max(0.0, seconds);
    if (!std::isfinite(seconds)) {
      return false;
    }
  }

  auto ts = static_cast<int64_t>(seconds * static_cast<double>(AV_TIME_BASE));
  if (avformat_seek_file(fmt_ctx_, -1, INT64_MIN, ts, INT64_MAX, 0) < 0) {
    return false;
  }
  avcodec_flush_buffers(codec_ctx_);
  leftover_.clear();
  leftover_offset_ = 0;
  total_output_frames_ = static_cast<size_t>(
      std::llround(seconds * static_cast<double>(output_sample_rate_)));
  return true;
}

size_t FFmpegDecoder::readPcmFrames(float *outInterleaved, size_t frameCount) {
  if (!isOpen() || outInterleaved == nullptr || frameCount == 0 || output_channels_ <= 0) {
    return 0;
  }
  size_t delivered = 0;
  const auto ch = static_cast<size_t>(output_channels_);

  while (delivered < frameCount) {
    size_t need = frameCount - delivered;
    size_t available_samples = leftover_.size() > leftover_offset_
        ? leftover_.size() - leftover_offset_
        : 0;
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
    } else if (!feedPipeline()) {
      break;
    }
  }
  total_output_frames_ += delivered;
  return delivered;
}

static std::shared_ptr<AudioBuffer> buildAudioBufferFromInterleaved(
    std::vector<float> &interleaved,
    int channels,
    int sample_rate) {
  if (interleaved.empty() || channels <= 0) {
    return nullptr;
  }
  size_t frames = interleaved.size() / static_cast<size_t>(channels);
  auto buf = std::make_shared<AudioBuffer>(frames, channels, static_cast<float>(sample_rate));
  buf->deinterleaveFrom(interleaved.data(), frames);
  return buf;
}

std::shared_ptr<AudioBuffer> decodeWithFilePath(const std::string &path, int sample_rate) {
  FFmpegDecoder dec;
  if (!dec.openFile(sample_rate, path)) {
    return nullptr;
  }
  std::vector<float> acc;
  std::vector<float> tmp(
      decoding::IIncrementalAudioDecoder::CHUNK_SIZE *
      static_cast<size_t>(std::max(1, dec.outputChannels())));
  while (true) {
    size_t n = dec.readPcmFrames(tmp.data(), decoding::IIncrementalAudioDecoder::CHUNK_SIZE);
    if (n == 0) {
      break;
    }
    acc.insert(
        acc.end(),
        tmp.begin(),
        tmp.begin() + static_cast<ptrdiff_t>(n * static_cast<size_t>(dec.outputChannels())));
  }
  return buildAudioBufferFromInterleaved(acc, dec.outputChannels(), dec.outputSampleRate());
}

std::shared_ptr<AudioBuffer> decodeWithMemoryBlock(const void *data, size_t size, int sample_rate) {
  FFmpegDecoder dec;
  if (!dec.openMemory(sample_rate, data, size)) {
    return nullptr;
  }
  std::vector<float> acc;
  std::vector<float> tmp(
      decoding::IIncrementalAudioDecoder::CHUNK_SIZE *
      static_cast<size_t>(std::max(1, dec.outputChannels())));
  while (true) {
    size_t n = dec.readPcmFrames(tmp.data(), decoding::IIncrementalAudioDecoder::CHUNK_SIZE);
    if (n == 0) {
      break;
    }
    acc.insert(
        acc.end(),
        tmp.begin(),
        tmp.begin() + static_cast<ptrdiff_t>(n * static_cast<size_t>(dec.outputChannels())));
  }
  return buildAudioBufferFromInterleaved(acc, dec.outputChannels(), dec.outputSampleRate());
}

} // namespace audioapi::ffmpegdecoder

#else

namespace audioapi::ffmpegdecoder {
FFmpegDecoder::~FFmpegDecoder() = default;
void FFmpegDecoder::close() {}
bool FFmpegDecoder::openFile(int, const std::string &) {
  return false;
}
bool FFmpegDecoder::openMemory(int, const void *, size_t) {
  return false;
}
float FFmpegDecoder::getDurationInSeconds() const {
  return 0;
}
float FFmpegDecoder::getCurrentPositionInSeconds() const {
  return 0;
}
bool FFmpegDecoder::seekToTime(double) {
  return false;
}
size_t FFmpegDecoder::readPcmFrames(float *, size_t) {
  return 0;
}
std::shared_ptr<AudioBuffer> decodeWithFilePath(const std::string &, int) {
  return nullptr;
}
std::shared_ptr<AudioBuffer> decodeWithMemoryBlock(const void *, size_t, int) {
  return nullptr;
}

} // namespace audioapi::ffmpegdecoder

#endif // !RN_AUDIO_API_FFMPEG_DISABLED
