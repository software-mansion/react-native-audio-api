#if !RN_AUDIO_API_FFMPEG_DISABLED

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <audioapi/android/core/utils/ffmpegBackend/utils.h>

#include <audioapi/android/core/utils/FileOptions.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <memory>
#include <string>

namespace audioapi::android::ffmpeg {

/// @brief Get the PCM codec ID based on the bit depth.
/// Note: This function returns only PCM codec IDs and its different from getSampleFormat. :)
/// @param properties The audio file properties.
/// @return The corresponding PCM AVCodecID.
AVCodecID getPCMCodecID(const std::shared_ptr<AudioFileProperties> &properties) {
  switch (properties->bitDepth) {
    case AudioFileProperties::BitDepth::Bit16:
      return AV_CODEC_ID_PCM_S16LE;
    case AudioFileProperties::BitDepth::Bit24:
      return AV_CODEC_ID_PCM_S24LE;
    case AudioFileProperties::BitDepth::Bit32:
      return AV_CODEC_ID_PCM_F32LE;
    default:
      return AV_CODEC_ID_PCM_F32LE;
  }
}

/// @brief Get the codec ID based on the audio file properties.
/// Note: PCM codec is used with wav and caf formats as both are uncompressed formats.
/// @param properties The audio file properties.
/// @return The corresponding AVCodecID.
AVCodecID getCodecID(const std::shared_ptr<AudioFileProperties> &properties) {
  switch (properties->format) {
    case AudioFileProperties::Format::WAV:
    case AudioFileProperties::Format::CAF:
      return getPCMCodecID(properties);
    case AudioFileProperties::Format::M4A:
      return AV_CODEC_ID_AAC;
    case AudioFileProperties::Format::FLAC:
      return AV_CODEC_ID_FLAC;
    default:
      return AV_CODEC_ID_AAC;
  }
}

/// @brief Returns the appropriate AVSampleFormat for codecs that require it. (non-PCM codecs)
/// @param properties The audio file properties.
/// @return The corresponding AVSampleFormat.
AVSampleFormat getSampleFormat(const std::shared_ptr<AudioFileProperties> &properties) {
  if (properties->format == AudioFileProperties::Format::M4A) {
    return AV_SAMPLE_FMT_FLTP;
  }

  switch (properties->bitDepth) {
    case AudioFileProperties::BitDepth::Bit16:
      return AV_SAMPLE_FMT_S16;
    case AudioFileProperties::BitDepth::Bit24:
      return AV_SAMPLE_FMT_S32;
    case AudioFileProperties::BitDepth::Bit32:
      return AV_SAMPLE_FMT_FLT;
    default:
      return AV_SAMPLE_FMT_FLT;
  }
}

/// @brief Finds the appropriate codec based on the audio file properties.
/// @param properties The audio file properties.
/// @return A pointer to the AVCodec.
const AVCodec *getCodec(const std::shared_ptr<AudioFileProperties> &properties) {
  return avcodec_find_encoder(getCodecID(properties));
}

/// @brief Returns the appropriate muxer name based on the audio file properties.
/// Note: most of the time, the muxer name is same as the file extension, (M4A uses MP4 muxer :))
/// thus this is kept separate from format -> extension mapping.
/// @param properties The audio file properties.
/// @return The corresponding muxer name.
std::string getMuxerName(const std::shared_ptr<AudioFileProperties> &properties) {
  switch (properties->format) {
    case AudioFileProperties::Format::WAV:
      return "wav";
    case AudioFileProperties::Format::CAF:
      return "caf";
    case AudioFileProperties::Format::M4A:
      return "mp4";
    case AudioFileProperties::Format::FLAC:
      return "flac";
    default:
      return "mp4";
  }
}

/// @brief Parses the FFmpeg error int code into a human-readable string.
/// @param errorCode The FFmpeg error code.
/// @return A human-readable string describing the error.
std::string parseErrorCode(int errorCode) {
  char errorBuffer[AV_ERROR_MAX_STRING_SIZE];

  if (av_strerror(errorCode, errorBuffer, sizeof(errorBuffer)) < 0) {
    return "Unknown FFmpeg error: " + std::to_string(errorCode);
  }

  return std::string(errorBuffer);
}

} // namespace audioapi::android::ffmpeg

#endif // RN_AUDIO_API_FFMPEG_DISABLED
