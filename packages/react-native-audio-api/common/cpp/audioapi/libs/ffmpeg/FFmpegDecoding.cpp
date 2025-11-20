/*
 * This file dynamically links to the FFmpeg library, which is licensed under
 * the GNU Lesser General Public License (LGPL) version 2.1 or later.
 *
 * Our own code in this file is licensed under the MIT License and dynamic
 * linking allows you to use this code without your entire project being subject
 * to the terms of the LGPL. However, note that if you link statically to
 * FFmpeg, you must comply with the terms of the LGPL for FFmpeg itself.
 */

#include <audioapi/core/sources/AudioBuffer.h>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <functional>

namespace audioapi::ffmpegdecoder {

int read_packet(void *opaque, uint8_t *buf, int buf_size) {
  MemoryIOContext *ctx = static_cast<MemoryIOContext *>(opaque);

  if (ctx->pos >= ctx->size) {
    return AVERROR_EOF;
  }

  int bytes_to_read =
      std::min(buf_size, static_cast<int>(ctx->size - ctx->pos));
  memcpy(buf, ctx->data + ctx->pos, bytes_to_read);
  ctx->pos += bytes_to_read;

  return bytes_to_read;
}

int64_t seek_packet(void *opaque, int64_t offset, int whence) {
  MemoryIOContext *ctx = static_cast<MemoryIOContext *>(opaque);

  switch (whence) {
    case SEEK_SET:
      ctx->pos = offset;
      break;
    case SEEK_CUR:
      ctx->pos += offset;
      break;
    case SEEK_END:
      ctx->pos = ctx->size + offset;
      break;
    case AVSEEK_SIZE:
      return ctx->size;
  }

  if (ctx->pos > ctx->size) {
    ctx->pos = ctx->size;
  }

  return ctx->pos;
}

void convertFrameToBuffer(
    SwrContext *swr,
    AVFrame *frame,
    int output_channel_count,
    std::vector<float> &buffer,
    size_t &framesRead,
    uint8_t **&resampled_data,
    int &max_resampled_samples) {
  const int out_samples = swr_get_out_samples(swr, frame->nb_samples);
  if (out_samples > max_resampled_samples) {
    av_freep(&resampled_data[0]);
    av_freep(&resampled_data);
    max_resampled_samples = out_samples;

    if (av_samples_alloc_array_and_samples(
            &resampled_data,
            nullptr,
            output_channel_count,
            max_resampled_samples,
            AV_SAMPLE_FMT_FLT,
            0) < 0) {
      return;
    }
  }

  int converted_samples = swr_convert(
      swr,
      resampled_data,
      max_resampled_samples,
      const_cast<const uint8_t **>(frame->data),
      frame->nb_samples);

  if (converted_samples > 0) {
    const size_t current_size = buffer.size();
    const size_t new_samples =
        static_cast<size_t>(converted_samples) * output_channel_count;
    buffer.resize(current_size + new_samples);
    memcpy(
        buffer.data() + current_size,
        resampled_data[0],
        new_samples * sizeof(float));
    framesRead += converted_samples;
  }
}

std::vector<float> readAllPcmFrames(
    AVFormatContext *fmt_ctx,
    AVCodecContext *codec_ctx,
    int out_sample_rate,
    int output_channel_count,
    int audio_stream_index,
    size_t &framesRead) {
  framesRead = 0;
  std::vector<float> buffer;
  auto swr = std::unique_ptr<SwrContext, std::function<void(SwrContext *)>>(
      swr_alloc(), [](SwrContext *ctx) { swr_free(&ctx); });

  if (swr == nullptr)
    return buffer;

  av_opt_set_chlayout(swr.get(), "in_chlayout", &codec_ctx->ch_layout, 0);
  av_opt_set_int(swr.get(), "in_sample_rate", codec_ctx->sample_rate, 0);
  av_opt_set_sample_fmt(swr.get(), "in_sample_fmt", codec_ctx->sample_fmt, 0);

  AVChannelLayout out_ch_layout;
  av_channel_layout_default(&out_ch_layout, output_channel_count);
  av_opt_set_chlayout(swr.get(), "out_chlayout", &out_ch_layout, 0);
  av_opt_set_int(swr.get(), "out_sample_rate", out_sample_rate, 0);
  av_opt_set_sample_fmt(swr.get(), "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

  if (swr_init(swr.get()) < 0) {
    av_channel_layout_uninit(&out_ch_layout);
    return buffer;
  }

  auto packet = std::unique_ptr<AVPacket, std::function<void(AVPacket *)>>(
      av_packet_alloc(), [](AVPacket *p) { av_packet_free(&p); });
  auto frame = std::unique_ptr<AVFrame, std::function<void(AVFrame *)>>(
      av_frame_alloc(), [](AVFrame *p) { av_frame_free(&p); });

  if (packet == nullptr || frame == nullptr) {
    av_channel_layout_uninit(&out_ch_layout);
    return buffer;
  }

  // Allocate buffer for resampled data
  uint8_t **resampled_data = nullptr;
  int max_resampled_samples = 4096; // Initial size
  if (av_samples_alloc_array_and_samples(
          &resampled_data,
          nullptr,
          output_channel_count,
          max_resampled_samples,
          AV_SAMPLE_FMT_FLT,
          0) < 0) {
    av_channel_layout_uninit(&out_ch_layout);
    return buffer;
  }

  while (av_read_frame(fmt_ctx, packet.get()) >= 0) {
    if (packet->stream_index == audio_stream_index) {
      if (avcodec_send_packet(codec_ctx, packet.get()) == 0) {
        while (avcodec_receive_frame(codec_ctx, frame.get()) == 0) {
          convertFrameToBuffer(
              swr.get(),
              frame.get(),
              output_channel_count,
              buffer,
              framesRead,
              resampled_data,
              max_resampled_samples);
        }
      }
    }
    av_packet_unref(packet.get());
  }

  // Flush decoder
  avcodec_send_packet(codec_ctx, nullptr);
  while (avcodec_receive_frame(codec_ctx, frame.get()) == 0) {
    convertFrameToBuffer(
        swr.get(),
        frame.get(),
        output_channel_count,
        buffer,
        framesRead,
        resampled_data,
        max_resampled_samples);
  }

  av_freep(&resampled_data[0]);
  av_freep(&resampled_data);
  av_channel_layout_uninit(&out_ch_layout);

  return buffer;
}

inline int findAudioStreamIndex(AVFormatContext *fmt_ctx) {
  for (int i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      return i;
    }
  }
  return -1;
}

bool setupDecoderContext(
    AVFormatContext *fmt_ctx,
    int &audio_stream_index,
    std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext *)>>
        &codec_ctx) {
  audio_stream_index = findAudioStreamIndex(fmt_ctx);
  if (audio_stream_index == -1) {
    return false;
  }

  AVCodecParameters *codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;
  const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
  if (codec == nullptr) {
    return false;
  }

  AVCodecContext *raw_codec_ctx = avcodec_alloc_context3(codec);
  if (raw_codec_ctx == nullptr) {
    return false;
  }

  codec_ctx.reset(raw_codec_ctx);
  if (avcodec_parameters_to_context(codec_ctx.get(), codecpar) < 0) {
    return false;
  }
  if (avcodec_open2(codec_ctx.get(), codec, nullptr) < 0) {
    return false;
  }

  return true;
}

std::shared_ptr<AudioBuffer> decodeAudioFrames(
    AVFormatContext *fmt_ctx,
    AVCodecContext *codec_ctx,
    int audio_stream_index,
    int sample_rate) {
  size_t framesRead = 0;
  int output_sample_rate =
      (sample_rate > 0) ? sample_rate : codec_ctx->sample_rate;
  int output_channel_count = codec_ctx->ch_layout.nb_channels;

  std::vector<float> decoded_buffer = readAllPcmFrames(
      fmt_ctx,
      codec_ctx,
      output_sample_rate,
      output_channel_count,
      audio_stream_index,
      framesRead);

  if (framesRead == 0 || decoded_buffer.empty()) {
    return nullptr;
  }

  auto outputFrames = decoded_buffer.size() / output_channel_count;
  auto audioBus = std::make_shared<AudioBus>(
      outputFrames, output_channel_count, output_sample_rate);

  for (int ch = 0; ch < output_channel_count; ++ch) {
    auto channelData = audioBus->getChannel(ch)->getData();
    for (int i = 0; i < outputFrames; ++i) {
      channelData[i] = decoded_buffer[i * output_channel_count + ch];
    }
  }
  return std::make_shared<AudioBuffer>(audioBus);
}

std::shared_ptr<AudioBuffer>
decodeWithMemoryBlock(const void *data, size_t size, int sample_rate) {
  if (data == nullptr || size == 0) {
    return nullptr;
  }

  MemoryIOContext io_ctx{static_cast<const uint8_t *>(data), size, 0};

  constexpr size_t buffer_size = 4096;
  uint8_t *io_buffer = static_cast<uint8_t *>(av_malloc(buffer_size));
  if (io_buffer == nullptr) {
    return nullptr;
  }

  auto avio_ctx =
      std::unique_ptr<AVIOContext, std::function<void(AVIOContext *)>>(
          avio_alloc_context(
              io_buffer,
              buffer_size,
              0,
              &io_ctx,
              read_packet,
              nullptr,
              seek_packet),
          [](AVIOContext *ctx) { avio_context_free(&ctx); });
  if (avio_ctx == nullptr) {
    return nullptr;
  }

  AVFormatContext *raw_fmt_ctx = avformat_alloc_context();
  if (raw_fmt_ctx == nullptr) {
    return nullptr;
  }
  raw_fmt_ctx->pb = avio_ctx.get();

  if (avformat_open_input(&raw_fmt_ctx, nullptr, nullptr, nullptr) < 0) {
    avformat_free_context(raw_fmt_ctx);
    return nullptr;
  }

  auto fmt_ctx =
      std::unique_ptr<AVFormatContext, decltype(&avformat_free_context)>(
          raw_fmt_ctx, &avformat_free_context);

  if (avformat_find_stream_info(fmt_ctx.get(), nullptr) < 0) {
    return nullptr;
  }

  auto codec_ctx =
      std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext *)>>(
          nullptr, [](AVCodecContext *ctx) { avcodec_free_context(&ctx); });
  int audio_stream_index = -1;
  if (!setupDecoderContext(fmt_ctx.get(), audio_stream_index, codec_ctx)) {
    return nullptr;
  }

  return decodeAudioFrames(
      fmt_ctx.get(), codec_ctx.get(), audio_stream_index, sample_rate);
}

std::shared_ptr<AudioBuffer> decodeWithFilePath(
    const std::string &path,
    int sample_rate) {
  if (path.empty()) {
    return nullptr;
  }

  AVFormatContext *raw_fmt_ctx = nullptr;
  if (avformat_open_input(&raw_fmt_ctx, path.c_str(), nullptr, nullptr) < 0)
    return nullptr;

  auto fmt_ctx =
      std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext *)>>(
          raw_fmt_ctx,
          [](AVFormatContext *ctx) { avformat_close_input(&ctx); });

  if (avformat_find_stream_info(fmt_ctx.get(), nullptr) < 0) {
    return nullptr;
  }

  auto codec_ctx =
      std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext *)>>(
          nullptr, [](AVCodecContext *ctx) { avcodec_free_context(&ctx); });
  int audio_stream_index = -1;
  if (!setupDecoderContext(fmt_ctx.get(), audio_stream_index, codec_ctx)) {
    return nullptr;
  }

  return decodeAudioFrames(
      fmt_ctx.get(), codec_ctx.get(), audio_stream_index, sample_rate);
}

} // namespace audioapi::ffmpegdecoder
