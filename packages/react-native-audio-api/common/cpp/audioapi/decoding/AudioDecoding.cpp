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
      .data = decoding::EncodedBytes(
          static_cast<const uint8_t *>(data), static_cast<const uint8_t *>(data) + size),
      .sampleRate = sampleRate};
}

// Container durations are estimates, trusted only up to this length so a
// corrupt header cannot force a huge allocation. Longer files grow past it.
constexpr float MAX_TRUSTED_ESTIMATE_SECONDS = 30.0F * 60.0F;
// Headroom over the estimate (1/64 ≈ 1.6%) for encoder padding and priming.
constexpr size_t ESTIMATE_HEADROOM_DIVISOR = 64;
// Capacity growth once the estimate is exceeded: x1.5.
constexpr size_t GROWTH_NUMERATOR = 3;
constexpr size_t GROWTH_DENOMINATOR = 2;
// Unused capacity up to 1/8 of the decoded length is trimmed in place (the
// allocation stays); more than that is worth an exact-size copy.
constexpr size_t IN_PLACE_TRIM_SLACK_DIVISOR = 8;

size_t initialDecodeCapacity(size_t estimatedFrames, float sampleRate, size_t chunkFrames) {
  const auto maxTrustedEstimate =
      static_cast<size_t>(std::max(1.0F, sampleRate) * MAX_TRUSTED_ESTIMATE_SECONDS);
  const size_t trustedEstimate = std::min(estimatedFrames, maxTrustedEstimate);
  return trustedEstimate + trustedEstimate / ESTIMATE_HEADROOM_DIVISOR + chunkFrames;
}

size_t grownDecodeCapacity(size_t currentCapacity, size_t requiredFrames) {
  return std::max(requiredFrames, currentCapacity * GROWTH_NUMERATOR / GROWTH_DENOMINATOR);
}

// Decodes straight into the returned planar buffer, sized up front from the
// decoder's frame estimate, so peak memory stays near one copy of the PCM.
AudioBufferResult decodeAll(decoding::AudioDecoderBackend &decoder) {
  constexpr size_t chunkFrames = decoding::AudioDecoderBackend::CHUNK_SIZE;
  decoder.setPreferThroughput(true);
  const int channels = std::max(1, decoder.outputChannels());
  const auto outputSampleRate = static_cast<float>(decoder.outputSampleRate());

  const size_t initialCapacity =
      initialDecodeCapacity(decoder.getTotalPcmFrameCount(), outputSampleRate, chunkFrames);
  auto audioBuffer = std::make_shared<AudioBuffer>(initialCapacity, channels, outputSampleRate);

  std::vector<float> chunk(chunkFrames * static_cast<size_t>(channels));
  size_t framesDecoded = 0;
  while (true) {
    const size_t framesRead = decoder.readPcmFrames(chunk.data(), chunkFrames);
    if (framesRead == 0) {
      break;
    }
    if (framesDecoded + framesRead > audioBuffer->getSize()) {
      audioBuffer->resize(grownDecodeCapacity(audioBuffer->getSize(), framesDecoded + framesRead));
    }
    audioBuffer->deinterleaveFrom(chunk.data(), framesDecoded, framesRead);
    framesDecoded += framesRead;
  }

  if (framesDecoded == 0) {
    return Err("Failed to decode any frames");
  }

  const size_t capacity = audioBuffer->getSize();
  if (capacity - framesDecoded <= framesDecoded / IN_PLACE_TRIM_SLACK_DIVISOR) {
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
