/*
 * This file dynamically links to the FFmpeg library, which is licensed under the
 * GNU Lesser General Public License (LGPL) version 2.1 or later.
 *
 * Our own code in this file is licensed under the MIT License and dynamic linking
 * allows you to use this code without your entire project being subject to the
 * terms of the LGPL. However, note that if you link statically to FFmpeg, you must
 * comply with the terms of the LGPL for FFmpeg itself.
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

namespace audioapi::ffmpegdecoding {
// Custom IO context for reading from memory
struct MemoryIOContext {
    const uint8_t *data;
    size_t size;
    size_t pos;
  };

int read_packet(void *opaque, uint8_t *buf, int buf_size);
int64_t seek_packet(void *opaque, int64_t offset, int whence);
std::vector<int16_t> readAllPcmFrames(AVFormatContext *fmt_ctx, AVCodecContext *codec_ctx, int out_sample_rate, int audio_stream_index, int channels, size_t &framesRead);
std::vector<int16_t> decodeWithMemoryBlock(const void *data, size_t size, const int channel_count, int sample_rate);
std::vector<int16_t> decodeWithFilePath(const std::string &path, const int channel_count, int sample_rate);

} // namespace audioapi::ffmpegdecoder