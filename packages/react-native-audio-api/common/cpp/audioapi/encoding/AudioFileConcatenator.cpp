#include <audioapi/core/utils/Constants.h>
#include <audioapi/decoding/DecoderFactory.h>
#include <audioapi/decoding/DecoderSource.h>
#include <audioapi/encoding/AudioFileConcatenator.h>
#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/OSEncoding.h>
#include <audioapi/encoding/OSRemux.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/FileSystem.hpp>
#include <audioapi/utils/Path.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

namespace {

constexpr size_t DECODE_CHUNK_FRAMES = 4096;
/// Covers the canonical 44-byte header and the filler chunk CoreAudio adds on iOS, which pads
/// the data chunk out to a 4 KiB offset.
constexpr uint64_t WAV_HEADER_ALLOWANCE_BYTES = 8192;
constexpr uint64_t MAX_RIFF_CHUNK_SIZE = std::numeric_limits<uint32_t>::max();
constexpr int DEFAULT_FLAC_COMPRESSION_LEVEL = 5;

AudioFileConcatResult validatePaths(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  if (inputPaths.empty()) {
    return Err("concatAudioFiles requires at least one input path.");
  }

  for (size_t i = 0; i < inputPaths.size(); ++i) {
    if (inputPaths[i].empty()) {
      return Err("concatAudioFiles input path at index " + std::to_string(i) + " is empty.");
    }

    if (path::hasNonFileProtocol(inputPaths[i])) {
      return Err(
          "concatAudioFiles input path at index " + std::to_string(i) +
          " must be a local file path or file:// URL.");
    }
  }

  if (outputPath.empty()) {
    return Err("concatAudioFiles requires an output path.");
  }

  if (path::hasNonFileProtocol(outputPath)) {
    return Err("concatAudioFiles output path must be a local file path or file:// URL.");
  }

  return Ok(outputPath);
}

using FileFormat = AudioFileProperties::FileFormat;

/// The formats concat can write. The output path's extension picks one, and every input must
/// carry the same extension.
constexpr std::array CONCAT_OUTPUT_FORMATS = {FileFormat::WAV, FileFormat::M4A, FileFormat::FLAC};

std::string extensionOf(FileFormat format) {
  return std::string(encoder_capabilities::specForFormat(format).extension);
}

std::optional<FileFormat> outputFormatForPath(const std::string &path) {
  for (const auto format : CONCAT_OUTPUT_FORMATS) {
    if (path::hasExtension(path, {extensionOf(format)})) {
      return format;
    }
  }
  return std::nullopt;
}

} // namespace

AudioFileReader::AudioFileReader(AudioFileReader &&other) noexcept
    : filePath_(std::move(other.filePath_)), decoder_(std::move(other.decoder_)) {}

AudioFileReader &AudioFileReader::operator=(AudioFileReader &&other) noexcept {
  if (this != &other) {
    close();
    filePath_ = std::move(other.filePath_);
    decoder_ = std::move(other.decoder_);
  }
  return *this;
}

AudioFileReader::~AudioFileReader() {
  close();
}

Result<AudioFileReader, std::string> AudioFileReader::open(const std::string &filePath) {
  AudioFileReader input;
  input.filePath_ = filePath;

  auto decoderResult =
      decoding::createDecoder(decoding::LocalFileSource{.path = filePath, .sampleRate = 0});
  if (decoderResult.is_err()) {
    return Err("Failed to open input file '" + filePath + "': " + decoderResult.unwrap_err());
  }

  input.decoder_ = std::move(decoderResult).unwrap();
  if (!input.decoder_->isOpen() || input.decoder_->outputSampleRate() == 0 ||
      input.decoder_->outputChannels() == 0) {
    input.close();
    return Err("Input file '" + filePath + "' is missing required audio parameters.");
  }

  return Ok(std::move(input));
}

size_t AudioFileReader::readPcmFrames(float *frames, size_t frameCount) {
  if (decoder_ == nullptr || frames == nullptr || frameCount == 0) {
    return 0;
  }
  return decoder_->readPcmFrames(frames, frameCount);
}

const std::string &AudioFileReader::filePath() const {
  return filePath_;
}

uint32_t AudioFileReader::sampleRate() const {
  return static_cast<uint32_t>(decoder_->outputSampleRate());
}

uint32_t AudioFileReader::channels() const {
  return static_cast<uint32_t>(decoder_->outputChannels());
}

uint64_t AudioFileReader::totalPcmFrames() const {
  return static_cast<uint64_t>(decoder_->getTotalPcmFrameCount());
}

void AudioFileReader::close() {
  if (decoder_ != nullptr) {
    decoder_->close();
    decoder_.reset();
  }
}

namespace {

AudioFileConcatResult validateCompatibleDecodedInput(
    const AudioFileReader &input,
    const AudioFileReader &reference) {
  if (input.sampleRate() != reference.sampleRate()) {
    return Err("Input file '" + input.filePath() + "' uses a different sample rate.");
  }

  if (input.channels() != reference.channels()) {
    return Err("Input file '" + input.filePath() + "' uses a different channel count.");
  }

  return Ok(input.filePath());
}

AudioFileConcatResult openAndValidateDecodedInputs(
    const std::vector<std::string> &inputPaths,
    std::vector<AudioFileReader> &inputs) {
  inputs.reserve(inputPaths.size());

  for (const auto &inputPath : inputPaths) {
    auto inputResult = AudioFileReader::open(inputPath);
    if (inputResult.is_err()) {
      return Err(inputResult.unwrap_err());
    }

    inputs.emplace_back(std::move(inputResult).unwrap());

    if (inputs.size() > 1) {
      auto validationResult = validateCompatibleDecodedInput(inputs.back(), inputs.front());
      if (validationResult.is_err()) {
        return validationResult;
      }
    }
  }

  return Ok(std::string());
}

AudioFileConcatResult validateRiffWaveOutputSize(
    std::vector<AudioFileReader> &inputs,
    uint64_t bytesPerFrame) {
  if (bytesPerFrame == 0) {
    return Err("Input files use an unsupported WAV sample format.");
  }

  uint64_t totalDataBytes = 0;
  for (auto &input : inputs) {
    const uint64_t frameCount = input.totalPcmFrames();
    if (frameCount == 0) {
      return Err("Failed to determine decoded frame count for '" + input.filePath() + "'.");
    }

    if (frameCount > (std::numeric_limits<uint64_t>::max() - totalDataBytes) / bytesPerFrame) {
      return Err(
          "concatAudioFiles WAV output exceeds the RIFF WAV size limit. Split the output or use a format with RF64/W64 support.");
    }

    totalDataBytes += frameCount * bytesPerFrame;
    if (WAV_HEADER_ALLOWANCE_BYTES + totalDataBytes + (totalDataBytes % 2) > MAX_RIFF_CHUNK_SIZE) {
      return Err(
          "concatAudioFiles WAV output exceeds the RIFF WAV size limit. Split the output or use a format with RF64/W64 support.");
    }
  }

  return Ok(std::string());
}

AudioFileConcatResult concatAudioFilesWithOsRemux(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  for (const auto &inputPath : inputPaths) {
    if (!path::hasExtension(inputPath, {extensionOf(FileFormat::M4A)})) {
      return Err("concatAudioFiles M4A output requires all input files to use the M4A extension.");
    }
  }

  return remuxConcatAudioFiles(inputPaths, outputPath);
}

EncoderSettings makeEncoderSettings(
    AudioFileProperties::FileFormat format,
    const AudioLayout &layout) {
  return EncoderSettings{
      .fileLayout = layout,
      .encoding =
          {
              .format = format,
              .bitRate = 0,
              // WAV keeps the decoded float32 samples as they are; FLAC is integer-only.
              .bitDepth = format == AudioFileProperties::FileFormat::WAV
                  ? AudioFileProperties::BitDepth::Bit32
                  : AudioFileProperties::BitDepth::Bit16,
              .flacCompressionLevel = DEFAULT_FLAC_COMPRESSION_LEVEL,
              .iosAudioQuality = AudioFileProperties::IOSAudioQuality::Max,
          },
  };
}

/// Decodes every input and re-encodes the frames into one @p format file. Only WAV and FLAC
/// take this path; the remaining formats are remuxed.
AudioFileConcatResult concatAudioFilesWithEncoder(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath,
    AudioFileProperties::FileFormat format,
    const ConcatEncoderFactory &createEncoder) {
  const EncoderOutputSpec outputSpec = encoder_capabilities::specForFormat(format);
  const bool isWav = format == AudioFileProperties::FileFormat::WAV;
  const std::string formatName = isWav ? "WAV" : "FLAC";

  for (const auto &inputPath : inputPaths) {
    if (!path::hasExtension(inputPath, {std::string(outputSpec.extension)})) {
      return Err(
          "concatAudioFiles " + formatName + " output requires all input files to use the " +
          formatName + " extension.");
    }
  }

  const std::string unavailableError =
      "concatAudioFiles " + formatName + " output requires iOS or Android.";
  if (!createEncoder) {
    return Err(unavailableError);
  }

  std::vector<AudioFileReader> inputs;
  auto inputValidationResult = openAndValidateDecodedInputs(inputPaths, inputs);
  if (inputValidationResult.is_err()) {
    return inputValidationResult;
  }

  const uint32_t sampleRate = inputs.front().sampleRate();
  const uint32_t channels = inputs.front().channels();

  if (isWav) {
    auto outputSizeResult = validateRiffWaveOutputSize(inputs, uint64_t{channels} * sizeof(float));
    if (outputSizeResult.is_err()) {
      return outputSizeResult;
    }
  }

  if (channels > static_cast<uint32_t>(MAX_CHANNEL_COUNT)) {
    return Err(
        "concatAudioFiles " + formatName + " output: channel count exceeds MAX_CHANNEL_COUNT.");
  }

  const AudioLayout layout{
      .sampleRate = static_cast<float>(sampleRate),
      .channelCount = static_cast<int>(channels),
  };
  auto encoderResult = createEncoder(makeEncoderSettings(format, layout));
  if (encoderResult.is_err()) {
    return Err(
        "concatAudioFiles " + formatName + " output has no encoder: " + encoderResult.unwrap_err());
  }
  auto encoder = std::move(encoderResult).unwrap();

  auto openResult = encoder->open(
      StreamFormat{
          .layout = layout,
          .maxFramesPerBuffer = DECODE_CHUNK_FRAMES,
      },
      outputSpec,
      outputPath);
  if (openResult.is_err()) {
    return Err(
        "Failed to open " + formatName + " output '" + outputPath +
        "': " + openResult.unwrap_err());
  }

  // The decoder reads interleaved; the encoder takes planar, so repack once per chunk.
  std::vector<float> buffer(DECODE_CHUNK_FRAMES * channels);
  AudioBuffer planar(
      DECODE_CHUNK_FRAMES, static_cast<int>(channels), static_cast<float>(sampleRate));
  std::array<const float *, MAX_CHANNEL_COUNT> planarChannels{};
  for (uint32_t channel = 0; channel < channels; ++channel) {
    planarChannels[channel] = planar.getChannel(channel)->begin();
  }
  for (auto &input : inputs) {
    while (true) {
      const size_t framesRead = input.readPcmFrames(buffer.data(), DECODE_CHUNK_FRAMES);
      if (framesRead == 0) {
        break;
      }

      planar.deinterleaveFrom(buffer.data(), framesRead);
      auto encodeResult = encoder->encode(planarChannels.data(), static_cast<int>(framesRead));
      if (encodeResult.is_err()) {
        // A truncated file at the requested path would pass for a finished one.
        encoder->close();
        file_system::removeFile(outputPath);
        return Err(
            "Failed to encode frames from '" + input.filePath() +
            "': " + encodeResult.unwrap_err());
      }
    }
  }

  auto closeResult = encoder->close();
  if (closeResult.is_err()) {
    file_system::removeFile(outputPath);
    return Err(
        "Failed to finalize " + formatName + " output '" + outputPath +
        "': " + closeResult.unwrap_err());
  }

  return Ok(outputPath);
}

ConcatEncoderFactory platformEncoderFactory() {
#if RN_AUDIO_API_HAS_OS_ENCODER
  return &createOsEncoder;
#else
  return {};
#endif
}

} // namespace

AudioFileConcatResult concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  return concatAudioFiles(inputPaths, outputPath, platformEncoderFactory());
}

AudioFileConcatResult concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath,
    const ConcatEncoderFactory &createEncoder) {
  std::vector<std::string> normalizedInputPaths;
  normalizedInputPaths.reserve(inputPaths.size());
  for (const auto &inputPath : inputPaths) {
    normalizedInputPaths.push_back(path::normalizeFilePath(inputPath));
  }

  const std::string normalizedOutputPath = path::normalizeFilePath(outputPath);

  auto pathValidationResult = validatePaths(normalizedInputPaths, normalizedOutputPath);
  if (pathValidationResult.is_err()) {
    return pathValidationResult;
  }

  // M4A remuxes the inputs' AAC packets through OS APIs. WAV and FLAC have no remux path on
  // either platform, so their inputs are decoded and re-encoded through the system encoder:
  // WAV as float32 PCM, FLAC losslessly.
  const auto outputFormat = outputFormatForPath(normalizedOutputPath);
  if (!outputFormat.has_value()) {
    return Err("concatAudioFiles supports WAV, M4A, and FLAC output.");
  }

  auto result = *outputFormat == FileFormat::M4A
      ? concatAudioFilesWithOsRemux(normalizedInputPaths, normalizedOutputPath)
      : concatAudioFilesWithEncoder(
            normalizedInputPaths, normalizedOutputPath, *outputFormat, createEncoder);
  return std::move(result).map([&outputPath](const std::string &) { return outputPath; });
}

} // namespace audioapi
