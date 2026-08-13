#include <audioapi/decoding/AudioDecoding.h>
#include <audioapi/decoding/DecoderFactory.h>
#include <audioapi/decoding/DecoderSource.h>
#include <audioapi/decoding/backends/AudioDecoderBackend.h>

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi::audiodecoding {

namespace {

template <typename Fn, typename Result>
concept DecoderOperation = std::invocable<Fn, decoding::AudioDecoderBackend &> &&
    std::same_as<std::invoke_result_t<Fn, decoding::AudioDecoderBackend &>, Result>;

template <typename Result, DecoderOperation<Result> Fn>
Result withDecoderFromSource(
    const decoding::DecoderSource &source,
    const char *errorMessage,
    Fn &&operation) {
  auto decoderResult = decoding::createDecoder(source);
  if (decoderResult.is_err()) {
    return Err(errorMessage);
  }

  auto decoder = std::move(decoderResult).unwrap();
  auto result = std::forward<Fn>(operation)(*decoder);
  decoder->close();
  if (result.is_err()) {
    return Err(errorMessage);
  }
  return result;
}

decoding::EncodedMemorySource
makeEncodedMemorySource(const void *data, size_t size, int sampleRate) {
  return decoding::EncodedMemorySource{
      .data = std::vector<uint8_t>(
          static_cast<const uint8_t *>(data), static_cast<const uint8_t *>(data) + size),
      .sampleRate = sampleRate};
}

AudioBufferResult decodeAll(decoding::AudioDecoderBackend &decoder) {
  const int channels = std::max(1, decoder.outputChannels());
  const auto outputSampleRate = static_cast<float>(decoder.outputSampleRate());

  std::vector<float> interleaved;
  std::vector<float> chunk(
      decoding::AudioDecoderBackend::CHUNK_SIZE * static_cast<size_t>(channels));

  while (true) {
    const size_t framesRead =
        decoder.readPcmFrames(chunk.data(), decoding::AudioDecoderBackend::CHUNK_SIZE);
    if (framesRead == 0) {
      break;
    }
    interleaved.insert(
        interleaved.end(),
        chunk.begin(),
        chunk.begin() + static_cast<std::ptrdiff_t>(framesRead * static_cast<size_t>(channels)));
  }

  if (interleaved.empty()) {
    return Err("Failed to decode any frames");
  }

  const size_t outputFrames = interleaved.size() / static_cast<size_t>(channels);
  auto audioBuffer = std::make_shared<AudioBuffer>(outputFrames, channels, outputSampleRate);
  audioBuffer->deinterleaveFrom(interleaved.data(), outputFrames);
  return Ok(std::move(audioBuffer));
}

AudioDurationResult probeDurationFromSource(
    const decoding::DecoderSource &source,
    const char *errorMessage) {
  return withDecoderFromSource<AudioDurationResult>(
      source, errorMessage, resolveDurationFromDecoder);
}

AudioBufferResult decodeFromSource(
    const decoding::DecoderSource &source,
    const char *errorMessage) {
  return withDecoderFromSource<AudioBufferResult>(source, errorMessage, decodeAll);
}

} // namespace

bool pathHasExtension(const std::string &path, const std::vector<std::string> &extensions) {
  return decoding::pathHasExtension(path, extensions);
}

bool isHttpUrl(const std::string &path) {
  return path.starts_with("http://") || path.starts_with("https://");
}

bool isValidDuration(float duration) {
  return duration >= 0.0F && std::isfinite(duration);
}

AudioDurationResult probeDurationWithFilePath(const std::string &path) {
  return probeDurationFromSource(
      decoding::LocalFileSource{.path = path, .sampleRate = 0},
      "Cannot read duration: file could not be decoded");
}

AudioDurationResult probeDurationWithMemory(const void *data, size_t size, int sampleRate) {
  const int sr = sampleRate != 0 ? sampleRate : 0;
  if (data == nullptr || size == 0) {
    return Err("Cannot read duration: audio data could not be decoded");
  }
  return probeDurationFromSource(
      makeEncodedMemorySource(data, size, sr),
      "Cannot read duration: audio data could not be decoded");
}

AudioDurationResult probeDurationWithUrl(
    const std::string &url,
    int sampleRate,
    const std::map<std::string, std::string> &headers) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
  if (url.empty()) {
    return Err("Failed to open URL with FFmpeg decoder: url is empty");
  }
  return probeDurationFromSource(
      decoding::RemoteUrlSource{.url = url, .httpHeaders = headers, .sampleRate = sampleRate},
      "Failed to open URL with FFmpeg decoder");
#else
  (void)url;
  (void)sampleRate;
  (void)headers;
  return Err("FFmpeg is disabled, cannot probe duration from URL");
#endif
}

AudioBufferResult decodeWithFilePath(const std::string &path, float sampleRate) {
  const int sr = static_cast<int>(sampleRate);
  return decodeFromSource(
      decoding::LocalFileSource{.path = path, .sampleRate = sr},
      "Cannot decode file: unsupported or invalid audio format");
}

AudioBufferResult decodeWithMemoryBlock(const void *data, size_t size, float sampleRate) {
  const int sr = static_cast<int>(sampleRate);
  if (data == nullptr || size == 0) {
    return Err("Cannot decode audio data: unsupported or invalid audio format");
  }
  return decodeFromSource(
      makeEncodedMemorySource(data, size, sr),
      "Cannot decode audio data: unsupported or invalid audio format");
}

AudioBufferResult decodeWithPCMInBase64(
    const std::string &data,
    float inputSampleRate,
    int inputChannelCount,
    bool interleaved) {
  return decodeFromSource(
      decoding::RawPcmBase64Source{
          .base64 = data,
          .channelCount = inputChannelCount,
          .interleaved = interleaved,
          .sampleRate = static_cast<int>(inputSampleRate)},
      "Cannot decode PCM base64 data");
}

} // namespace audioapi::audiodecoding
