#include <audioapi/decoding/AudioDecodeStream.h>
#include <audioapi/decoding/DecoderSource.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi::audiodecoding {

namespace {

/// The highest rate the Web Audio API lets a context run at.
constexpr float MAX_OUTPUT_SAMPLE_RATE = 768000.0F;
constexpr auto INVALID_SAMPLE_RATE_MESSAGE = "Cannot open a decode stream: invalid sample rate";

/// Whether @p sampleRate can be handed to a decoder: `0` keeps the rate of the source.
/// Converting a NaN, an infinity or a value out of range to `int` is undefined behavior.
bool isOutputSampleRate(float sampleRate) {
  return std::isfinite(sampleRate) && sampleRate >= 0.0F && sampleRate <= MAX_OUTPUT_SAMPLE_RATE;
}

} // namespace

AudioDecodeStream::OpenResult AudioDecodeStream::openWithFilePath(
    const std::string &path,
    float sampleRate) {
  if (!isOutputSampleRate(sampleRate)) {
    return Err(INVALID_SAMPLE_RATE_MESSAGE);
  }
  return open(
      decoding::LocalFileSource{.path = path, .sampleRate = static_cast<int>(sampleRate)},
      0,
      "Cannot decode file: unsupported or invalid audio format");
}

AudioDecodeStream::OpenResult AudioDecodeStream::openWithMemoryBlock(
    std::vector<uint8_t> encodedBytes,
    float sampleRate) {
  constexpr auto errorMessage = "Cannot decode audio data: unsupported or invalid audio format";
  if (!isOutputSampleRate(sampleRate)) {
    return Err(INVALID_SAMPLE_RATE_MESSAGE);
  }
  if (encodedBytes.empty()) {
    return Err(errorMessage);
  }
  const size_t size = encodedBytes.size();
  return open(
      decoding::EncodedMemorySource{
          .data = std::move(encodedBytes), .sampleRate = static_cast<int>(sampleRate)},
      size,
      errorMessage);
}

AudioDecodeStream::OpenResult AudioDecodeStream::open(
    const decoding::DecoderSource &source,
    size_t encodedSizeInBytes,
    const char *errorMessage) {
  auto decoderResult = decoding::createDecoder(source);
  if (decoderResult.is_err()) {
    return Err(errorMessage);
  }
  return Ok(
      std::unique_ptr<AudioDecodeStream>(
          new AudioDecodeStream(std::move(decoderResult).unwrap(), encodedSizeInBytes)));
}

AudioDecodeStream::AudioDecodeStream(decoding::DecoderPtr decoder, size_t encodedSizeInBytes)
    : decoder_(std::move(decoder)),
      numberOfChannels_(std::max(1, decoder_->outputChannels())),
      sampleRate_(static_cast<float>(decoder_->outputSampleRate())),
      duration_(resolveDurationFromDecoder(*decoder_).unwrap_or(0.0F)),
      encodedSizeInBytes_(encodedSizeInBytes) {
  // A stream is read as fast as its consumer asks, like a whole-file decode, never paced by
  // playback.
  decoder_->setPreferThroughput(true);
}

AudioDecodeStream::~AudioDecodeStream() {
  close();
}

AudioBufferResult AudioDecodeStream::read(size_t frameCount) {
  if (decoder_ == nullptr) {
    return Err("Cannot read from a closed decode stream");
  }
  if (frameCount == 0) {
    return Err("Cannot read: frameCount must be positive");
  }
  return Ok(decodeFrames(*decoder_, frameCount));
}

AudioDecodeStream::SeekResult AudioDecodeStream::seek(double seconds) {
  if (decoder_ == nullptr) {
    return Err("Cannot seek in a closed decode stream");
  }
  if (!std::isfinite(seconds) || seconds < 0.0) {
    return Err("Cannot seek: position must be a non-negative number of seconds");
  }
  auto result = decoder_->seekToTime(seconds);
  if (result.is_err()) {
    return Err("Cannot seek: " + result.unwrap_err());
  }
  return Ok(None);
}

void AudioDecodeStream::close() {
  if (decoder_ == nullptr) {
    return;
  }
  decoder_->close();
  decoder_.reset();
}

} // namespace audioapi::audiodecoding
