#include <audioapi/core/utils/Constants.h>
#include <audioapi/decoding/DecoderFactory.h>
#include <audioapi/decoding/DecoderSource.h>
#include <audioapi/encoding/AdtsHeader.h>
#include <audioapi/encoding/AudioFileConcatenator.h>
#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/OSEncoding.h>
#include <audioapi/encoding/OSRemux.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/FileSystem.hpp>
#include <audioapi/utils/Path.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <random>
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

using ValidationResult = Result<NoneType, std::string>;
using FileFormat = AudioFileProperties::FileFormat;

ValidationResult validatePaths(
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

  return Ok(None);
}

/// The formats concat can write. The output path's extension picks one, and every input must
/// carry the same extension.
constexpr std::array CONCAT_OUTPUT_FORMATS = {
    FileFormat::WAV,
    FileFormat::M4A,
    FileFormat::FLAC,
    FileFormat::ADTS};

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

/// A sibling of @p outputPath under a name no file holds yet. The extension is kept because
/// the iOS writers pick the container from it.
std::string makeStagingPath(const std::string &outputPath) {
  const std::filesystem::path finalPath(outputPath);
  const std::string stem = finalPath.stem().string();
  const std::string extension = finalPath.extension().string();
  std::random_device entropy;
  while (true) {
    std::filesystem::path candidate = finalPath;
    candidate.replace_filename(stem + ".partial-" + std::to_string(entropy()) + extension);
    if (!file_system::fileExists(candidate.string())) {
      return candidate.string();
    }
  }
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

ValidationResult validateCompatibleDecodedInput(
    const AudioFileReader &input,
    const AudioFileReader &reference) {
  if (input.sampleRate() != reference.sampleRate()) {
    return Err("Input file '" + input.filePath() + "' uses a different sample rate.");
  }

  if (input.channels() != reference.channels()) {
    return Err("Input file '" + input.filePath() + "' uses a different channel count.");
  }

  return Ok(None);
}

/// Opens every input; all of them must share the first one's sample rate and channel count.
Result<std::vector<AudioFileReader>, std::string> openCompatibleInputs(
    const std::vector<std::string> &inputPaths) {
  std::vector<AudioFileReader> inputs;
  inputs.reserve(inputPaths.size());

  for (const auto &inputPath : inputPaths) {
    auto inputResult = AudioFileReader::open(inputPath);
    if (inputResult.is_err()) {
      return Err(std::move(inputResult).unwrap_err());
    }

    inputs.emplace_back(std::move(inputResult).unwrap());

    if (inputs.size() > 1) {
      auto validationResult = validateCompatibleDecodedInput(inputs.back(), inputs.front());
      if (validationResult.is_err()) {
        return Err(std::move(validationResult).unwrap_err());
      }
    }
  }

  return Ok(std::move(inputs));
}

ValidationResult validateRiffWaveOutputSize(
    const std::vector<AudioFileReader> &inputs,
    uint64_t bytesPerFrame) {
  if (bytesPerFrame == 0) {
    return Err("Input files use an unsupported WAV sample format.");
  }

  uint64_t totalDataBytes = 0;
  for (const auto &input : inputs) {
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

  return Ok(None);
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

EncoderSettings makeEncoderSettings(FileFormat format, const AudioLayout &layout) {
  return EncoderSettings{
      .fileLayout = layout,
      .encoding =
          {
              .format = format,
              .bitRate = 0,
              // WAV keeps the decoded float32 samples as they are; FLAC is integer-only.
              .bitDepth = format == FileFormat::WAV ? AudioFileProperties::BitDepth::Bit32
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
    FileFormat format,
    const EncoderFactory &createEncoder) {
  const EncoderOutputSpec outputSpec = encoder_capabilities::specForFormat(format);
  const bool isWav = format == FileFormat::WAV;
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

  auto inputsResult = openCompatibleInputs(inputPaths);
  if (inputsResult.is_err()) {
    return Err(std::move(inputsResult).unwrap_err());
  }
  auto inputs = std::move(inputsResult).unwrap();

  const uint32_t sampleRate = inputs.front().sampleRate();
  const uint32_t channels = inputs.front().channels();

  if (isWav) {
    auto outputSizeResult = validateRiffWaveOutputSize(inputs, uint64_t{channels} * sizeof(float));
    if (outputSizeResult.is_err()) {
      return Err(std::move(outputSizeResult).unwrap_err());
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

/// What a walk over an ADTS input found: the layout its frames declare and how many bytes
/// hold complete frames. A recording cut off by a crash ends in a partial frame, which is
/// left behind so the frame after it, from the next input, is not swallowed by a desync.
struct AdtsInputInfo {
  uint8_t samplingFrequencyIndex = 0;
  uint8_t channelConfiguration = 0;
  std::streamoff completeFrameBytes = 0;
};

constexpr size_t COPY_CHUNK_BYTES = 64 * 1024;

Result<AdtsInputInfo, std::string> scanAdtsFrames(const std::string &inputPath) {
  std::ifstream input(inputPath, std::ios::binary | std::ios::ate);
  if (!input.is_open()) {
    return Err("Failed to open input file '" + inputPath + "'.");
  }
  const std::streamoff fileSize = input.tellg();

  std::optional<adts::FrameInfo> firstFrame;
  std::streamoff offset = 0;
  while (offset + static_cast<std::streamoff>(adts::HEADER_SIZE) <= fileSize) {
    adts::HeaderBytes header{};
    input.seekg(offset);
    input.read(
        reinterpret_cast<char *>(header.data()), static_cast<std::streamsize>(header.size()));
    if (!input) {
      return Err("Failed to read input file '" + inputPath + "'.");
    }

    const auto frame = adts::parseHeader(header);
    if (!frame.has_value()) {
      return Err(
          "Input file '" + inputPath + "' is not an ADTS stream (no frame at byte " +
          std::to_string(offset) + ").");
    }
    if (firstFrame.has_value() &&
        (frame->samplingFrequencyIndex != firstFrame->samplingFrequencyIndex ||
         frame->channelConfiguration != firstFrame->channelConfiguration)) {
      return Err(
          "Input file '" + inputPath + "' changes its sample rate or channel count mid-stream.");
    }
    if (!firstFrame.has_value()) {
      firstFrame = frame;
    }

    const auto frameEnd = offset + static_cast<std::streamoff>(frame->frameLength);
    if (frameEnd > fileSize) {
      break;
    }
    offset = frameEnd;
  }

  if (!firstFrame.has_value()) {
    return Err("Input file '" + inputPath + "' contains no ADTS frames.");
  }
  return Ok(
      AdtsInputInfo{
          .samplingFrequencyIndex = firstFrame->samplingFrequencyIndex,
          .channelConfiguration = firstFrame->channelConfiguration,
          .completeFrameBytes = offset});
}

/// Appends the first @p byteCount bytes of @p inputPath to @p output.
ValidationResult
appendLeadingBytes(const std::string &inputPath, std::streamoff byteCount, std::ofstream &output) {
  std::ifstream input(inputPath, std::ios::binary);
  if (!input.is_open()) {
    return Err("Failed to open input file '" + inputPath + "'.");
  }

  std::vector<char> chunk(COPY_CHUNK_BYTES);
  std::streamoff remaining = byteCount;
  while (remaining > 0) {
    const auto toRead = static_cast<std::streamsize>(
        std::min<std::streamoff>(remaining, static_cast<std::streamoff>(chunk.size())));
    input.read(chunk.data(), toRead);
    const std::streamsize bytesRead = input.gcount();
    if (bytesRead <= 0) {
      return Err("Failed to read input file '" + inputPath + "'.");
    }
    output.write(chunk.data(), bytesRead);
    if (!output) {
      return Err("Failed to write the output file.");
    }
    remaining -= bytesRead;
  }
  return Ok(None);
}

/// ADTS has no container, so the inputs' frames follow each other unchanged. Every input must
/// be an ADTS stream with the first one's sample rate and channel configuration; nothing is
/// decoded, so this works on every platform.
AudioFileConcatResult concatAudioFilesWithFrameAppend(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  for (const auto &inputPath : inputPaths) {
    if (!path::hasExtension(inputPath, {extensionOf(FileFormat::ADTS)})) {
      return Err("concatAudioFiles ADTS output requires all input files to use the AAC extension.");
    }
  }

  std::vector<AdtsInputInfo> inputs;
  inputs.reserve(inputPaths.size());
  for (const auto &inputPath : inputPaths) {
    auto infoResult = scanAdtsFrames(inputPath);
    if (infoResult.is_err()) {
      return Err(std::move(infoResult).unwrap_err());
    }
    inputs.push_back(std::move(infoResult).unwrap());

    const auto &reference = inputs.front();
    if (inputs.back().samplingFrequencyIndex != reference.samplingFrequencyIndex) {
      return Err("Input file '" + inputPath + "' uses a different sample rate.");
    }
    if (inputs.back().channelConfiguration != reference.channelConfiguration) {
      return Err("Input file '" + inputPath + "' uses a different channel count.");
    }
  }

  std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
  if (!output.is_open()) {
    return Err("Failed to open ADTS output '" + outputPath + "'.");
  }

  for (size_t i = 0; i < inputPaths.size(); ++i) {
    auto copyResult = appendLeadingBytes(inputPaths[i], inputs[i].completeFrameBytes, output);
    if (copyResult.is_err()) {
      output.close();
      file_system::removeFile(outputPath);
      return Err(std::move(copyResult).unwrap_err());
    }
  }

  output.close();
  if (!output) {
    file_system::removeFile(outputPath);
    return Err("Failed to finalize ADTS output '" + outputPath + "'.");
  }
  return Ok(outputPath);
}

EncoderFactory platformEncoderFactory() {
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
    const EncoderFactory &createEncoder) {
  std::vector<std::string> normalizedInputPaths;
  normalizedInputPaths.reserve(inputPaths.size());
  for (const auto &inputPath : inputPaths) {
    normalizedInputPaths.push_back(path::normalizeFilePath(inputPath));
  }

  const std::string normalizedOutputPath = path::normalizeFilePath(outputPath);

  auto pathValidationResult = validatePaths(normalizedInputPaths, normalizedOutputPath);
  if (pathValidationResult.is_err()) {
    return Err(std::move(pathValidationResult).unwrap_err());
  }

  // M4A remuxes the inputs' AAC packets through OS APIs. ADTS frames are self-delimiting, so
  // its inputs are appended byte for byte. WAV and FLAC have no remux path on either platform,
  // so their inputs are decoded and re-encoded through the system encoder: WAV as float32
  // PCM, FLAC losslessly.
  const auto outputFormat = outputFormatForPath(normalizedOutputPath);
  if (!outputFormat.has_value()) {
    return Err("concatAudioFiles supports WAV, M4A, FLAC, and ADTS output.");
  }

  // The output is written under a staging name and moved into place once complete, so a failure
  // never leaves a truncated file at the output path and the output may be one of the inputs.
  const std::string stagingPath = makeStagingPath(normalizedOutputPath);
  AudioFileConcatResult result = Err(std::string{});
  switch (*outputFormat) {
    case FileFormat::M4A:
      result = concatAudioFilesWithOsRemux(normalizedInputPaths, stagingPath);
      break;
    case FileFormat::ADTS:
      result = concatAudioFilesWithFrameAppend(normalizedInputPaths, stagingPath);
      break;
    default:
      result = concatAudioFilesWithEncoder(
          normalizedInputPaths, stagingPath, *outputFormat, createEncoder);
      break;
  }
  if (result.is_err()) {
    file_system::removeFile(stagingPath);
    return Err(std::move(result).unwrap_err());
  }

  const std::error_code moveError = file_system::moveFile(stagingPath, normalizedOutputPath);
  if (moveError) {
    file_system::removeFile(stagingPath);
    return Err(
        "Failed to move the concatenated file to '" + normalizedOutputPath +
        "': " + moveError.message());
  }
  return Ok(outputPath);
}

} // namespace audioapi
