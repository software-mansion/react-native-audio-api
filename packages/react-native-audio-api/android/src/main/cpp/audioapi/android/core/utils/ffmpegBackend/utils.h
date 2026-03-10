#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <memory>
#include <string>

namespace audioapi {

class AudioFileProperties;

namespace android::ffmpeg {

template <typename AVT>
struct AvDtor {
  void operator()(AVT *ptr) const;
};

template <typename AVT>
using av_unique_ptr = std::unique_ptr<AVT, AvDtor<AVT>>;

AVCodecID getPCMCodecID(const std::shared_ptr<AudioFileProperties> &properties);
AVCodecID getCodecID(const std::shared_ptr<AudioFileProperties> &properties);
AVSampleFormat getSampleFormat(const std::shared_ptr<AudioFileProperties> &properties);
const AVCodec *getCodec(const std::shared_ptr<AudioFileProperties> &properties);
std::string getMuxerName(const std::shared_ptr<AudioFileProperties> &properties);

std::string parseErrorCode(int errorCode);

} // namespace android::ffmpeg

} // namespace audioapi
