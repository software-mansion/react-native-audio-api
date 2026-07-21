#include <audioapi/decoding/DecoderFactory.h>
#include <audioapi/decoding/OSDecoding.h>
#include <audioapi/decoding/backends/MiniAudioDecoder.h>
#include <audioapi/decoding/backends/RawPcmDecoder.h>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/decoding/backends/FfmpegDecoder.h>
#endif

#include <concepts>
#include <memory>
#include <utility>

namespace audioapi::decoding {
namespace {

// Combines multiple lambdas into one visitor so std::visit can dispatch on DecoderSource
// (each lambda handles a different std::variant alternative).
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#if RN_AUDIO_API_HAS_OS_DECODER
template <typename Source>
concept OsFallbackOpenableSource = requires(
    const Source &source,
    miniaudio::MiniAudioDecoder &miniAudioDecoder,
    os_decoder::Decoder &platformDecoder) {
  { miniAudioDecoder.open(source) } -> std::same_as<DecoderResult>;
  { platformDecoder.open(source) } -> std::same_as<DecoderResult>;
};
#else
template <typename Source>
concept OsFallbackOpenableSource =
    requires(const Source &source, miniaudio::MiniAudioDecoder &miniAudioDecoder) {
      { miniAudioDecoder.open(source) } -> std::same_as<DecoderResult>;
    };
#endif

template <typename Decoder, typename Source>
CreateDecoderResult createAndOpen(const Source &source) {
  auto decoder = std::make_unique<Decoder>();
  if (auto opened = decoder->open(source); opened.is_err()) {
    decoder->close();
    return Err(opened.unwrap_err());
  }
  return Ok(DecoderPtr(std::move(decoder)));
}

template <OsFallbackOpenableSource Source>
CreateDecoderResult tryOpenWithOsFallback(const Source &source, const char *errorMessage) {
#if RN_AUDIO_API_HAS_OS_DECODER
  if (auto result = createAndOpen<os_decoder::Decoder>(source); result.is_ok()) {
    return result;
  }
#endif

  if (auto result = createAndOpen<miniaudio::MiniAudioDecoder>(source); result.is_ok()) {
    return result;
  }
  return Err(errorMessage);
}

} // namespace

CreateDecoderResult createDecoder(const DecoderSource &source) {
  return std::visit(
      overloaded{
          [](const LocalFileSource &localSource) -> CreateDecoderResult {
            if (localSource.path.empty()) {
              return Err("DecoderFactory: local file path is empty");
            }
            if (localPathRequiresFfmpeg(localSource.path)) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
              return createAndOpen<ffmpeg::FfmpegDecoder>(localSource);
#else
              return Err("FFmpeg is disabled, cannot decode local HLS (.m3u8) playlists");
#endif
            }
            return tryOpenWithOsFallback(
                localSource, "Cannot open local file: unsupported or invalid audio format");
          },
          [](const EncodedMemorySource &memorySource) -> CreateDecoderResult {
            if (memorySource.data.empty()) {
              return Err("DecoderFactory: encoded memory source is empty");
            }
            return tryOpenWithOsFallback(
                memorySource, "Cannot decode encoded memory: unsupported or invalid audio format");
          },
          [](const RemoteUrlSource &remoteSource) -> CreateDecoderResult {
            if (remoteSource.url.empty()) {
              return Err("DecoderFactory: remote URL is empty");
            }
#if !RN_AUDIO_API_FFMPEG_DISABLED
            return createAndOpen<ffmpeg::FfmpegDecoder>(remoteSource);
#else
            (void)remoteSource;
            return Err("FFmpeg is disabled, cannot decode remote URL");
#endif
          },
          [](const RawPcmSource &pcmSource) -> CreateDecoderResult {
            return createAndOpen<raw_pcm::RawPcmDecoder>(pcmSource);
          },
          [](const RawPcmBase64Source &pcmSource) -> CreateDecoderResult {
            return createAndOpen<raw_pcm::RawPcmDecoder>(pcmSource);
          },
      },
      source);
}

} // namespace audioapi::decoding
