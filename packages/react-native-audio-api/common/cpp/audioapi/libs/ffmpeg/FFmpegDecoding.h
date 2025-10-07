/*
 * This file dynamically links to the FFmpeg library, which is licensed under
 * the GNU Lesser General Public License (LGPL) version 2.1 or later.
 *
 * Our own code in this file is licensed under the MIT License and dynamic
 * linking allows you to use this code without your entire project being subject
 * to the terms of the LGPL. However, note that if you link statically to
 * FFmpeg, you must comply with the terms of the LGPL for FFmpeg itself.
 */

#include <audioapi/utils/AudioBus.h>
#include <iostream>
#include <memory>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}
class AudioBuffer;

namespace audioapi::ffmpegdecoder {
// Custom IO context for reading from memory
struct MemoryIOContext {
  const uint8_t *data;
  size_t size;
  size_t pos;
};

struct AudioStreamContext {
  AVFormatContext *fmt_ctx = nullptr;
  AVCodecContext *codec_ctx = nullptr;
  int audio_stream_index = -1;
};

int read_packet(void *opaque, uint8_t *buf, int buf_size);
int64_t seek_packet(void *opaque, int64_t offset, int whence);
inline int findAudioStreamIndex(AVFormatContext *fmt_ctx);
std::vector<float> readAllPcmFrames(
    AVFormatContext *fmt_ctx,
    AVCodecContext *codec_ctx,
    int out_sample_rate,
    int output_channel_count,
    int audio_stream_index,
    size_t &framesRead);

void convertFrameToBuffer(
    SwrContext *swr,
    AVFrame *frame,
    int output_channel_count,
    std::vector<float> &buffer,
    size_t &framesRead,
    uint8_t **&resampled_data,
    int &max_resampled_samples);
bool setupDecoderContext(
    AVFormatContext *fmt_ctx,
    int &audio_stream_index,
    std::unique_ptr<AVCodecContext, decltype(&avcodec_free_context)>
        &codec_ctx);
std::shared_ptr<AudioBuffer> decodeAudioFrames(
    AVFormatContext *fmt_ctx,
    AVCodecContext *codec_ctx,
    int audio_stream_index,
    int sample_rate);

std::shared_ptr<AudioBuffer>
decodeWithMemoryBlock(const void *data, size_t size, int sample_rate);

std::shared_ptr<AudioBuffer> decodeWithFilePath(
    const std::string &path,
    int sample_rate);

} // namespace audioapi::ffmpegdecoder
