#include <audioapi/core/utils/RecordingFileName.h>
#include <audioapi/encoding/AudioEncoding.h>
#include <audioapi/utils/FileSystem.hpp>
#include <audioapi/utils/Path.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi::audioencoding {

namespace {

constexpr size_t ENCODE_CHUNK_FRAMES = 4096;

StreamFormat inputFormatOf(const AudioBuffer &buffer) {
  return StreamFormat{
      .layout =
          {.sampleRate = buffer.getSampleRate(),
           .channelCount = static_cast<int>(buffer.getNumberOfChannels())},
      .maxFramesPerBuffer = ENCODE_CHUNK_FRAMES,
  };
}

/// A sample rate or channel count of 0 means "not set": it takes the first buffer's, capped at
/// what a file can hold, since the encoder resamples and mixes down anything larger.
AudioLayout resolveFileLayout(AudioLayout requested, const AudioBuffer &first) {
  if (requested.sampleRate == 0) {
    requested.sampleRate =
        std::min(first.getSampleRate(), AudioFileProperties::MAX_FILE_SAMPLE_RATE);
  }
  if (requested.channelCount == 0) {
    requested.channelCount = std::min(
        static_cast<int>(first.getNumberOfChannels()), AudioFileProperties::MAX_FILE_CHANNEL_COUNT);
  }
  return requested;
}

Result<NoneType, std::string> encodeAll(
    const std::vector<std::shared_ptr<const AudioBuffer>> &buffers,
    AudioEncoder &encoder) {
  StreamFormat currentFormat = inputFormatOf(*buffers.front());
  AudioBuffer chunk(
      ENCODE_CHUNK_FRAMES, currentFormat.layout.channelCount, currentFormat.layout.sampleRate);

  for (size_t i = 0; i < buffers.size(); ++i) {
    const auto &buffer = *buffers[i];
    const StreamFormat bufferFormat = inputFormatOf(buffer);

    /// A buffer whose layout differs from the one before it switches the encoder's input, which
    /// resamples and remixes it to the file's layout; the file itself stays open.
    if (bufferFormat != currentFormat) {
      auto changeResult = encoder.changeInputFormat(bufferFormat);
      if (changeResult.is_err()) {
        return Err(
            "Failed to switch to the layout of the buffer at index " + std::to_string(i) + ": " +
            changeResult.unwrap_err());
      }
      currentFormat = bufferFormat;
      chunk = AudioBuffer(
          ENCODE_CHUNK_FRAMES, currentFormat.layout.channelCount, currentFormat.layout.sampleRate);
    }

    for (size_t offset = 0; offset < buffer.getSize(); offset += ENCODE_CHUNK_FRAMES) {
      const size_t frames = std::min(ENCODE_CHUNK_FRAMES, buffer.getSize() - offset);
      chunk.copy(buffer, offset, 0, frames);
      auto result = encoder.encode(chunk, static_cast<int>(frames));
      if (result.is_err()) {
        return Err("Failed to encode audio data: " + result.unwrap_err());
      }
    }
  }
  return Ok(None);
}

} // namespace

EncodeAudioDataResult encodeAudioData(
    const std::vector<std::shared_ptr<const AudioBuffer>> &buffers,
    const std::shared_ptr<AudioFileProperties> &properties,
    const PlatformFileBackend &backend) {
  if (buffers.empty()) {
    return Err("encodeAudioData requires at least one AudioBuffer.");
  }
  properties->fileLayout = resolveFileLayout(properties->fileLayout, *buffers.front());
  auto validationResult = properties->validate();
  if (validationResult.is_err()) {
    return Err(std::move(validationResult).unwrap_err());
  }

  const std::string stem = properties->path.fileName.empty()
      ? "encoded_" + recording_file_name::sessionTimestamp()
      : properties->path.fileName;
  auto encoderResult = openFileEncoder(backend, properties, stem, inputFormatOf(*buffers.front()));
  if (encoderResult.is_err()) {
    return Err(std::move(encoderResult).unwrap_err());
  }
  auto encoder = std::move(encoderResult).unwrap();
  const std::string filePath = encoder->getFilePath();

  auto encodeResult = encodeAll(buffers, *encoder);
  auto closeResult = encoder->close();
  if (encodeResult.is_err() || closeResult.is_err()) {
    file_system::removeFile(filePath);
    return Err(
        encodeResult.is_err()
            ? std::move(encodeResult).unwrap_err()
            : "Failed to finalize '" + filePath + "': " + closeResult.unwrap_err());
  }

  const auto &[sizeMB, durationSeconds] = closeResult.unwrap();
  return Ok(
      EncodedFileInfo{
          .path = path::toFileUrl(filePath), .size = sizeMB, .duration = durationSeconds});
}

} // namespace audioapi::audioencoding
