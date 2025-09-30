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
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi::ffmpegdecoding {

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

std::vector<float> readAllPcmFrames(
    AVFormatContext *fmt_ctx,
    AVCodecContext *codec_ctx,
    int out_sample_rate,
    int output_channel_count,
    int audio_stream_index,
    size_t &framesRead) {
  std::vector<float> buffer;
  SwrContext *swr_ctx = swr_alloc();
  if (swr_ctx == nullptr) {
    return buffer;
  }

  av_opt_set_chlayout(swr_ctx, "in_chlayout", &codec_ctx->ch_layout, 0);
  av_opt_set_int(swr_ctx, "in_sample_rate", codec_ctx->sample_rate, 0);
  av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codec_ctx->sample_fmt, 0);

  AVChannelLayout out_ch_layout;
  av_channel_layout_default(&out_ch_layout, output_channel_count);
  av_opt_set_chlayout(swr_ctx, "out_chlayout", &out_ch_layout, 0);
  av_opt_set_int(swr_ctx, "out_sample_rate", out_sample_rate, 0);
  av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

  if (swr_init(swr_ctx) < 0) {
    swr_free(&swr_ctx);
    av_channel_layout_uninit(&out_ch_layout);
    return buffer;
  }

  AVPacket *packet = av_packet_alloc();
  AVFrame *frame = av_frame_alloc();

  if (packet == nullptr || frame == nullptr) {
    if (packet != nullptr)
      av_packet_free(&packet);
    if (frame != nullptr)
      av_frame_free(&frame);
    swr_free(&swr_ctx);
    av_channel_layout_uninit(&out_ch_layout);
    return buffer;
  }

  // Allocate buffer for resampled data
  uint8_t **resampled_data = nullptr;
  int max_resampled_samples = 4096; // Initial size
  int ret = av_samples_alloc_array_and_samples(
      &resampled_data,
      nullptr,
      output_channel_count,
      max_resampled_samples,
      AV_SAMPLE_FMT_FLT,
      0);

  if (ret < 0) {
    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swr_ctx);
    av_channel_layout_uninit(&out_ch_layout);
    return buffer;
  }

  framesRead = 0;

  while (av_read_frame(fmt_ctx, packet) >= 0) {
    if (packet->stream_index == audio_stream_index) {
      if (avcodec_send_packet(codec_ctx, packet) == 0) {
        while (avcodec_receive_frame(codec_ctx, frame) == 0) {
          // Check if we need more buffer space
          int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
          if (out_samples > max_resampled_samples) {
            if (resampled_data != nullptr) {
              av_freep(&resampled_data[0]);
              av_freep(&resampled_data);
            }

            max_resampled_samples = out_samples;
            ret = av_samples_alloc_array_and_samples(
                &resampled_data,
                nullptr,
                output_channel_count,
                max_resampled_samples,
                AV_SAMPLE_FMT_FLT,
                0);

            if (ret < 0) {
              break; // Exit on allocation failure
            }
          }

          int converted_samples = swr_convert(
              swr_ctx,
              resampled_data,
              max_resampled_samples,
              (const uint8_t **)frame->data,
              frame->nb_samples);

          if (converted_samples > 0) {
            size_t current_size = buffer.size();
            size_t new_samples = converted_samples * output_channel_count;
            buffer.resize(current_size + new_samples);
            memcpy(
                buffer.data() + current_size,
                resampled_data[0],
                new_samples * sizeof(float));

            framesRead += converted_samples;
          }
        }
      }
    }
    av_packet_unref(packet);
  }

  // Flush decoder
  avcodec_send_packet(codec_ctx, nullptr);
  while (avcodec_receive_frame(codec_ctx, frame) == 0) {
    int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
    if (out_samples > max_resampled_samples) {
      if (resampled_data != nullptr) {
        av_freep(&resampled_data[0]);
        av_freep(&resampled_data);
      }

      max_resampled_samples = out_samples;
      ret = av_samples_alloc_array_and_samples(
          &resampled_data,
          nullptr,
          output_channel_count,
          max_resampled_samples,
          AV_SAMPLE_FMT_FLT,
          0);

      if (ret < 0) {
        break;
      }
    }

    int converted_samples = swr_convert(
        swr_ctx,
        resampled_data,
        max_resampled_samples,
        (const uint8_t **)frame->data,
        frame->nb_samples);

    if (converted_samples > 0) {
      size_t current_size = buffer.size();
      size_t new_samples = converted_samples * output_channel_count;
      buffer.resize(current_size + new_samples);
      memcpy(
          buffer.data() + current_size,
          resampled_data[0],
          new_samples * sizeof(float));

      framesRead += converted_samples;
    }
  }

  if (resampled_data != nullptr) {
    av_freep(&resampled_data[0]);
    av_freep(&resampled_data);
  }
  swr_free(&swr_ctx);
  av_channel_layout_uninit(&out_ch_layout);
  av_frame_free(&frame);
  av_packet_free(&packet);

  return buffer;
}

std::shared_ptr<AudioBuffer> makeAudioBufferFromFloatBuffer(
    const std::vector<float> &buffer,
    float outputSampleRate,
    int outputChannels) {
  if (buffer.empty()) {
    return nullptr;
  }

  auto outputFrames = buffer.size() / outputChannels;
  auto audioBus = std::make_shared<AudioBus>(
      outputFrames, outputChannels, outputSampleRate);

  for (int ch = 0; ch < outputChannels; ++ch) {
    auto channelData = audioBus->getChannel(ch)->getData();
    for (int i = 0; i < outputFrames; ++i) {
      channelData[i] = buffer[i * outputChannels + ch];
    }
  }
  return std::make_shared<AudioBuffer>(audioBus);
}

std::shared_ptr<AudioBuffer>
decodeWithMemoryBlock(const void *data, size_t size, int sample_rate) {
  if (data == nullptr || size == 0) {
    return nullptr;
  }

  MemoryIOContext io_ctx;
  io_ctx.data = static_cast<const uint8_t *>(data);
  io_ctx.size = size;
  io_ctx.pos = 0;

  constexpr size_t buffer_size = 4096;
  uint8_t *io_buffer = static_cast<uint8_t *>(av_malloc(buffer_size));
  if (io_buffer == nullptr) {
    return nullptr;
  }

  AVIOContext *avio_ctx = avio_alloc_context(
      io_buffer, buffer_size, 0, &io_ctx, read_packet, nullptr, seek_packet);

  if (avio_ctx == nullptr) {
    av_free(io_buffer);
    return nullptr;
  }

  // Create format context and set custom IO
  AVFormatContext *fmt_ctx = avformat_alloc_context();
  if (fmt_ctx == nullptr) {
    avio_context_free(&avio_ctx);
    return nullptr;
  }

  fmt_ctx->pb = avio_ctx;

  // Open input from memory
  if (avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr) < 0) {
    avformat_free_context(fmt_ctx);
    avio_context_free(&avio_ctx);
    return nullptr;
  }

  // Find stream info
  if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
    avformat_close_input(&fmt_ctx);
    avio_context_free(&avio_ctx);
    return nullptr;
  }

  int audio_stream_index = -1;
  for (int i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index = i;
      break;
    }
  }

  if (audio_stream_index == -1) {
    avformat_close_input(&fmt_ctx);
    avio_context_free(&avio_ctx);
    return nullptr;
  }

  AVCodecParameters *codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;

  // Find decoder
  const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
  if (codec == nullptr) {
    avformat_close_input(&fmt_ctx);
    avio_context_free(&avio_ctx);
    return nullptr;
  }

  // Allocate and setup codec context
  AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
  if (codec_ctx == nullptr) {
    avformat_close_input(&fmt_ctx);
    avio_context_free(&avio_ctx);
    return nullptr;
  }

  if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    avio_context_free(&avio_ctx);
    return nullptr;
  }

  if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    avio_context_free(&avio_ctx);
    return nullptr;
  }

  // Decode all frames
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

  // Cleanup - Note: avio_context_free will free the io_buffer
  avcodec_free_context(&codec_ctx);
  avformat_close_input(&fmt_ctx);
  avio_context_free(&avio_ctx);

  if (framesRead == 0 || decoded_buffer.empty()) {
    return nullptr;
  }

  return makeAudioBufferFromFloatBuffer(
      decoded_buffer, output_sample_rate, output_channel_count);
}

std::shared_ptr<AudioBuffer> decodeWithFilePath(
    const std::string &path,
    int sample_rate) {
  if (path.empty()) {
    return nullptr;
  }

  AVFormatContext *fmt_ctx = nullptr;
  if (avformat_open_input(&fmt_ctx, path.c_str(), nullptr, nullptr) < 0) {
    return nullptr;
  }
  if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
    avformat_close_input(&fmt_ctx);
    return nullptr;
  }
  int audio_stream_index = -1;
  for (int i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index = i;
      break;
    }
  }
  if (audio_stream_index == -1) {
    avformat_close_input(&fmt_ctx);
    return nullptr;
  }

  AVCodecParameters *codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;
  const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
  if (codec == nullptr) {
    avformat_close_input(&fmt_ctx);
    return nullptr;
  }
  AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
  if (codec_ctx == nullptr) {
    avformat_close_input(&fmt_ctx);
    return nullptr;
  }

  if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return nullptr;
  }

  if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return nullptr;
  }

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

  avcodec_free_context(&codec_ctx);
  avformat_close_input(&fmt_ctx);

  if (framesRead == 0 || decoded_buffer.empty()) {
    return nullptr;
  }

  return makeAudioBufferFromFloatBuffer(
      decoded_buffer, output_sample_rate, output_channel_count);
}

} // namespace audioapi::ffmpegdecoding
