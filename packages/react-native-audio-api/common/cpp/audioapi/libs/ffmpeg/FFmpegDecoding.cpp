/*
 * This file dynamically links to the FFmpeg library, which is licensed under
 * the GNU Lesser General Public License (LGPL) version 2.1 or later.
 *
 * Our own code in this file is licensed under the MIT License and dynamic
 * linking allows you to use this code without your entire project being subject
 * to the terms of the LGPL. However, note that if you link statically to
 * FFmpeg, you must comply with the terms of the LGPL for FFmpeg itself.
 */

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>

#include <algorithm>
#include <cstring>
#include <utility>

extern "C" {
#include <libavutil/channel_layout.h>
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

FFmpegDecoder::FFmpegDecoder(FFmpegDecoder &&other) noexcept {
  *this = std::move(other);
}

FFmpegDecoder &FFmpegDecoder::operator=(FFmpegDecoder &&other) noexcept {
  if (this != &other) {
    close();
    fmt_ctx_ = other.fmt_ctx_;
    codec_ctx_ = other.codec_ctx_;
    swr_ = other.swr_;
    packet_ = other.packet_;
    frame_ = other.frame_;
    resampled_data_ = other.resampled_data_;
    max_resampled_samples_ = other.max_resampled_samples_;
    mem_io_ = std::move(other.mem_io_);
    avio_ctx_ = other.avio_ctx_;
    leftover_ = std::move(other.leftover_);
    audio_stream_index_ = other.audio_stream_index_;
    output_channels_ = other.output_channels_;
    output_sample_rate_ = other.output_sample_rate_;

    other.fmt_ctx_ = nullptr;
    other.codec_ctx_ = nullptr;
    other.swr_ = nullptr;
    other.packet_ = nullptr;
    other.frame_ = nullptr;
    other.resampled_data_ = nullptr;
    other.max_resampled_samples_ = 0;
    other.avio_ctx_ = nullptr;
  }
  return *this;
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
  audio_stream_index_ = -1;
  output_channels_ = 0;
  output_sample_rate_ = 0;
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
          FFmpegDecoder::CHUNK_SIZE,
          AV_SAMPLE_FMT_FLT,
          0) < 0) {
    return false;
  }
  max_resampled_samples_ = FFmpegDecoder::CHUNK_SIZE;
  return true;
}

bool FFmpegDecoder::openFile(const FFmpegDecoderConfig &cfg, const std::string &path) {
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
      (cfg.outputSampleRate > 0) ? cfg.outputSampleRate : codec_ctx_->sample_rate;

  packet_ = av_packet_alloc();
  frame_ = av_frame_alloc();
  if (packet_ == nullptr || frame_ == nullptr || !setupSwr()) {
    close();
    return false;
  }
  return true;
}

bool FFmpegDecoder::openMemory(const FFmpegDecoderConfig &cfg, const void *data, size_t size) {
  close();
  if (data == nullptr || size == 0) {
    return false;
  }
  mem_io_ = std::make_unique<MemoryIOContext>();
  mem_io_->data = static_cast<const uint8_t *>(data);
  mem_io_->size = size;
  mem_io_->pos = 0;

  auto* io_buf = static_cast<uint8_t *>(av_malloc(FFmpegDecoder::CHUNK_SIZE));
  if (io_buf == nullptr) {
    close();
    return false;
  }
  avio_ctx_ = avio_alloc_context(
    io_buf,
    static_cast<int>(FFmpegDecoder::CHUNK_SIZE),
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
      (cfg.outputSampleRate > 0) ? cfg.outputSampleRate : codec_ctx_->sample_rate;

  packet_ = av_packet_alloc();
  frame_ = av_frame_alloc();
  if (packet_ == nullptr || frame_ == nullptr || !setupSwr()) {
    close();
    return false;
  }
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

size_t FFmpegDecoder::readPcmFrames(float *outInterleaved, size_t frameCount) {
  if (!isOpen() || outInterleaved == nullptr || frameCount == 0 || output_channels_ <= 0) {
    return 0;
  }
  size_t delivered = 0;
  const auto ch = static_cast<size_t>(output_channels_);

  while (delivered < frameCount) {
    size_t need = frameCount - delivered;
    size_t leftover_frames = leftover_.size() / ch;
    if (leftover_frames > 0) {
      size_t take = std::min(need, leftover_frames);
      size_t samples = take * ch;
      memcpy(
          outInterleaved + delivered * ch,
          leftover_.data(),
          samples * sizeof(float));
      leftover_.erase(leftover_.begin(), leftover_.begin() + static_cast<ptrdiff_t>(samples));
      delivered += take;
    } else if (!feedPipeline()) {
      break;
    }
  }
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
  for (int c = 0; c < channels; ++c) {
    auto span = buf->getChannel(c)->span();
    for (size_t i = 0; i < frames; ++i) {
      span[i] = interleaved[i * static_cast<size_t>(channels) + static_cast<size_t>(c)];
    }
  }
  return buf;
}

std::shared_ptr<AudioBuffer> decodeWithFilePath(const std::string &path, int sample_rate) {
  FFmpegDecoderConfig cfg;
  ffmpegDecoderConfigInit(&cfg, sample_rate);
  FFmpegDecoder dec;
  if (!dec.openFile(cfg, path)) {
    return nullptr;
  }
  std::vector<float> acc;
  std::vector<float> tmp(FFmpegDecoder::CHUNK_SIZE * static_cast<size_t>(std::max(1, dec.outputChannels())));
  while (true) {
    size_t n = dec.readPcmFrames(tmp.data(), FFmpegDecoder::CHUNK_SIZE);
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
  FFmpegDecoderConfig cfg;
  ffmpegDecoderConfigInit(&cfg, sample_rate);
  FFmpegDecoder dec;
  if (!dec.openMemory(cfg, data, size)) {
    return nullptr;
  }
  std::vector<float> acc;
  std::vector<float> tmp(FFmpegDecoder::CHUNK_SIZE * static_cast<size_t>(std::max(1, dec.outputChannels())));
  while (true) {
    size_t n = dec.readPcmFrames(tmp.data(), FFmpegDecoder::CHUNK_SIZE);
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

#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>

namespace audioapi::ffmpegdecoder {

FFmpegDecoder::FFmpegDecoder(FFmpegDecoder &&) noexcept = default;
FFmpegDecoder &FFmpegDecoder::operator=(FFmpegDecoder &&) noexcept = default;
FFmpegDecoder::~FFmpegDecoder() = default;
void FFmpegDecoder::close() {}
bool FFmpegDecoder::openFile(const FFmpegDecoderConfig &, const std::string &) {
  return false;
}
bool FFmpegDecoder::openMemory(const FFmpegDecoderConfig &, const void *, size_t) {
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
