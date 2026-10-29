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

constexpr float MAX_TRUSTED_ESTIMATE_SECONDS = 30.0F * 60.0F;

// Decodes straight into the returned planar buffer, sized up front from the
// decoder's frame estimate, so peak memory stays near one copy of the PCM.
AudioBufferResult decodeAll(decoding::AudioDecoderBackend &decoder) {
  constexpr size_t chunkFrames = decoding::AudioDecoderBackend::CHUNK_SIZE;
  decoder.preferThroughput();
  const int channels = std::max(1, decoder.outputChannels());
  const auto outputSampleRate = static_cast<float>(decoder.outputSampleRate());

  // Container durations are estimates; leave headroom for encoder padding, and
  // trust them only up to half an hour so a corrupt header cannot force a huge
  // allocation. Longer files grow geometrically past that.
  const size_t estimatedFrames = decoder.getTotalPcmFrameCount();
  const auto maxTrustedEstimate =
      static_cast<size_t>(std::max(1.0F, outputSampleRate) * MAX_TRUSTED_ESTIMATE_SECONDS);
  const size_t trustedEstimate = std::min(estimatedFrames, maxTrustedEstimate);
  const size_t initialCapacity = trustedEstimate + trustedEstimate / 64 + chunkFrames;
  auto audioBuffer = std::make_shared<AudioBuffer>(initialCapacity, channels, outputSampleRate);

  std::vector<float> chunk(chunkFrames * static_cast<size_t>(channels));
  size_t framesDecoded = 0;
  while (true) {
    const size_t framesRead = decoder.readPcmFrames(chunk.data(), chunkFrames);
    if (framesRead == 0) {
      break;
    }
    if (framesDecoded + framesRead > audioBuffer->getSize()) {
      audioBuffer->resize(std::max(framesDecoded + framesRead, audioBuffer->getSize() * 3 / 2));
    }
    audioBuffer->deinterleaveFrom(chunk.data(), framesDecoded, framesRead);
    framesDecoded += framesRead;
  }

  if (framesDecoded == 0) {
    return Err("Failed to decode any frames");
  }

  const size_t capacity = audioBuffer->getSize();
  // Shrinking in place keeps the allocation, so only trim small slack that
  // way; a badly overestimated duration gets an exact-size copy instead.
  if (capacity - framesDecoded <= framesDecoded / 8) {
    audioBuffer->resize(framesDecoded);
  } else {
    auto exact = std::make_shared<AudioBuffer>(framesDecoded, channels, outputSampleRate);
    exact->copy(*audioBuffer, 0, 0, framesDecoded);
    audioBuffer = std::move(exact);
  }
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
  if (url.empty()) {
    return Err("Failed to open URL with FFmpeg decoder: url is empty");
  }
  return probeDurationFromSource(
      decoding::RemoteUrlSource{.url = url, .httpHeaders = headers, .sampleRate = sampleRate},
      "Failed to open URL with FFmpeg decoder");
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
