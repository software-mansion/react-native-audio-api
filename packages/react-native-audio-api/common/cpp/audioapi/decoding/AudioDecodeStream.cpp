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

AudioDecodeStream::OpenResult AudioDecodeStream::openWithFilePath(
    const std::string &path,
    float sampleRate) {
  return open(
      decoding::LocalFileSource{.path = path, .sampleRate = static_cast<int>(sampleRate)},
      0,
      "Cannot decode file: unsupported or invalid audio format");
}

AudioDecodeStream::OpenResult
AudioDecodeStream::openWithMemoryBlock(const void *data, size_t size, float sampleRate) {
  constexpr auto errorMessage = "Cannot decode audio data: unsupported or invalid audio format";
  if (data == nullptr || size == 0) {
    return Err(errorMessage);
  }
  const auto *bytes = static_cast<const uint8_t *>(data);
  return open(
      decoding::EncodedMemorySource{
          .data = std::vector<uint8_t>(bytes, bytes + size),
          .sampleRate = static_cast<int>(sampleRate)},
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
