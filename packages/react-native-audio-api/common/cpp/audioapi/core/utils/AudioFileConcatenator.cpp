#include <audioapi/core/utils/AudioFileConcatenator.h>
#include <audioapi/encoding/OSRemux.h>
#include <audioapi/libs/miniaudio/miniaudio.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace audioapi {

namespace {

constexpr const char *fileUrlPrefix = "file://";
constexpr ma_uint64 miniaudioChunkFrames = 4096;
constexpr ma_uint64 riffWaveHeaderBytes = 36;
constexpr ma_uint64 maxRiffChunkSize = std::numeric_limits<uint32_t>::max();

bool hasNonFileProtocol(const std::string &path) {
  const auto colon = path.find(':');
  if (colon == std::string::npos) {
    return false;
  }

  const auto firstSlash = path.find('/');
  return firstSlash == std::string::npos || colon < firstSlash;
}

std::string percentDecode(const std::string &path) {
  std::string decoded;
  decoded.reserve(path.size());

  for (size_t i = 0; i < path.size(); ++i) {
    if (path[i] != '%' || i + 2 >= path.size()) {
      decoded.push_back(path[i]);
      continue;
    }

    unsigned int value = 0;
    auto first = path.data() + i + 1;
    auto last = first + 2;
    auto result = std::from_chars(first, last, value, 16);
    if (result.ec != std::errc() || result.ptr != last) {
      decoded.push_back(path[i]);
      continue;
    }

    decoded.push_back(static_cast<char>(value));
    i += 2;
  }

  return decoded;
}

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

    if (hasNonFileProtocol(inputPaths[i])) {
      return Err(
          "concatAudioFiles input path at index " + std::to_string(i) +
          " must be a local file path or file:// URL.");
    }
  }

  if (outputPath.empty()) {
    return Err("concatAudioFiles requires an output path.");
  }

  if (hasNonFileProtocol(outputPath)) {
    return Err("concatAudioFiles output path must be a local file path or file:// URL.");
  }

  return Ok(outputPath);
}

std::string lowercaseExtension(const std::string &path) {
  const auto dotIndex = path.find_last_of('.');
  if (dotIndex == std::string::npos) {
    return "";
  }

  std::string extension = path.substr(dotIndex + 1);
  std::ranges::transform(extension, extension.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  return extension;
}

bool hasExtension(const std::string &path, const std::vector<std::string> &extensions) {
  const std::string extension = lowercaseExtension(path);
  return std::ranges::find(extensions, extension) != extensions.end();
}

bool isMiniaudioOutputPath(const std::string &path) {
  return hasExtension(path, {"wav"});
}

// M4A/MP4 remux via OS APIs. WAV stays on the miniaudio PCM path above.
bool isOsRemuxOutputPath(const std::string &path) {
  return hasExtension(path, {"m4a", "mp4"});
}

bool extensionsCompatibleForRemux(const std::string &inputPath, const std::string &outputPath) {
  const std::string inExt = lowercaseExtension(inputPath);
  const std::string outExt = lowercaseExtension(outputPath);
  if (inExt == outExt) {
    return true;
  }
  // M4A and MP4 share the MPEG-4 audio container family.
  const bool inMpeg4 = inExt == "m4a" || inExt == "mp4";
  const bool outMpeg4 = outExt == "m4a" || outExt == "mp4";
  return inMpeg4 && outMpeg4;
}

std::string parseMiniAudioError(ma_result errorCode) {
  return {ma_result_description(errorCode)};
}

} // namespace

MiniAudioDecoderGuard::MiniAudioDecoderGuard(MiniAudioDecoderGuard &&other) noexcept
    : filePath_(std::move(other.filePath_)),
      decoder_(std::move(other.decoder_)),
      initialized_(std::exchange(other.initialized_, false)) {}

MiniAudioDecoderGuard &MiniAudioDecoderGuard::operator=(MiniAudioDecoderGuard &&other) noexcept {
  if (this != &other) {
    close();
    filePath_ = std::move(other.filePath_);
    decoder_ = std::move(other.decoder_);
    initialized_ = std::exchange(other.initialized_, false);
  }
  return *this;
}

MiniAudioDecoderGuard::~MiniAudioDecoderGuard() {
  close();
}

Result<MiniAudioDecoderGuard, std::string> MiniAudioDecoderGuard::open(
    const std::string &filePath,
    ma_format outputFormat) {
  MiniAudioDecoderGuard input;
  input.filePath_ = filePath;
  input.decoder_ = std::make_unique<ma_decoder>();

  ma_decoder_config config = ma_decoder_config_init(outputFormat, 0, 0);
  const ma_result result = ma_decoder_init_file(filePath.c_str(), &config, input.decoder_.get());
  if (result != MA_SUCCESS) {
    return Err(
        "Failed to open input file '" + filePath +
        "' with miniaudio: " + parseMiniAudioError(result));
  }

  input.initialized_ = true;
  if (input.decoder_->outputSampleRate == 0 || input.decoder_->outputChannels == 0) {
    return Err("Input file '" + filePath + "' is missing required audio parameters.");
  }

  return Ok(std::move(input));
}

ma_decoder *MiniAudioDecoderGuard::get() {
  return decoder_.get();
}

const std::string &MiniAudioDecoderGuard::filePath() const {
  return filePath_;
}

ma_uint32 MiniAudioDecoderGuard::sampleRate() const {
  return decoder_->outputSampleRate;
}

ma_uint32 MiniAudioDecoderGuard::channels() const {
  return decoder_->outputChannels;
}

ma_format MiniAudioDecoderGuard::format() const {
  return decoder_->outputFormat;
}

void MiniAudioDecoderGuard::close() {
  if (initialized_ && decoder_ != nullptr) {
    ma_decoder_uninit(decoder_.get());
    initialized_ = false;
  }
  decoder_.reset();
}

MiniAudioEncoderGuard::~MiniAudioEncoderGuard() {
  close();
}

AudioFileConcatResult MiniAudioEncoderGuard::open(
    const std::string &outputPath,
    ma_format format,
    ma_uint32 sampleRate,
    ma_uint32 channels) {
  ma_encoder_config config =
      ma_encoder_config_init(ma_encoding_format_wav, format, channels, sampleRate);

  const ma_result result = ma_encoder_init_file(outputPath.c_str(), &config, &encoder_);
  if (result != MA_SUCCESS) {
    return Err(
        "Failed to open output file '" + outputPath +
        "' with miniaudio: " + parseMiniAudioError(result));
  }

  initialized_ = true;
  return Ok(outputPath);
}

AudioFileConcatResult MiniAudioEncoderGuard::write(
    const std::string &inputPath,
    const void *frames,
    ma_uint64 frameCount) {
  ma_uint64 framesWritten = 0;
  const ma_result result =
      ma_encoder_write_pcm_frames(&encoder_, frames, frameCount, &framesWritten);
  if (result != MA_SUCCESS) {
    return Err(
        "Failed to write decoded frames from '" + inputPath +
        "' with miniaudio: " + parseMiniAudioError(result));
  }

  if (framesWritten != frameCount) {
    return Err("Failed to write all decoded frames from '" + inputPath + "' with miniaudio.");
  }

  return Ok(inputPath);
}

void MiniAudioEncoderGuard::close() {
  if (initialized_) {
    ma_encoder_uninit(&encoder_);
    initialized_ = false;
  }
}

namespace {

AudioFileConcatResult validateCompatibleMiniAudioInput(
    const MiniAudioDecoderGuard &input,
    const MiniAudioDecoderGuard &reference) {
  if (input.sampleRate() != reference.sampleRate()) {
    return Err("Input file '" + input.filePath() + "' uses a different sample rate.");
  }

  if (input.channels() != reference.channels()) {
    return Err("Input file '" + input.filePath() + "' uses a different channel count.");
  }

  if (input.format() != reference.format()) {
    return Err("Input file '" + input.filePath() + "' uses a different sample format.");
  }

  return Ok(input.filePath());
}

AudioFileConcatResult openAndValidateMiniAudioInputs(
    const std::vector<std::string> &inputPaths,
    std::vector<MiniAudioDecoderGuard> &inputs,
    ma_format outputFormat = ma_format_unknown) {
  inputs.reserve(inputPaths.size());

  for (const auto &inputPath : inputPaths) {
    auto inputResult = MiniAudioDecoderGuard::open(inputPath, outputFormat);
    if (inputResult.is_err()) {
      return Err(inputResult.unwrap_err());
    }

    inputs.emplace_back(std::move(inputResult).unwrap());

    if (inputs.size() > 1) {
      auto validationResult = validateCompatibleMiniAudioInput(inputs.back(), inputs.front());
      if (validationResult.is_err()) {
        return validationResult;
      }
    }
  }

  return Ok(std::string());
}

AudioFileConcatResult validateRiffWaveOutputSize(
    std::vector<MiniAudioDecoderGuard> &inputs,
    ma_uint64 bytesPerFrame) {
  if (bytesPerFrame == 0) {
    return Err("Input files use an unsupported WAV sample format.");
  }

  ma_uint64 totalDataBytes = 0;
  for (auto &input : inputs) {
    ma_uint64 frameCount = 0;
    const ma_result result = ma_decoder_get_length_in_pcm_frames(input.get(), &frameCount);
    if (result != MA_SUCCESS) {
      return Err(
          "Failed to determine decoded frame count for '" + input.filePath() +
          "' with miniaudio: " + parseMiniAudioError(result));
    }

    if (frameCount > (std::numeric_limits<ma_uint64>::max() - totalDataBytes) / bytesPerFrame) {
      return Err(
          "concatAudioFiles WAV output exceeds the RIFF WAV size limit. Split the output or use a format with RF64/W64 support.");
    }

    totalDataBytes += frameCount * bytesPerFrame;
    if (riffWaveHeaderBytes + totalDataBytes + (totalDataBytes & 1) > maxRiffChunkSize) {
      return Err(
          "concatAudioFiles WAV output exceeds the RIFF WAV size limit. Split the output or use a format with RF64/W64 support.");
    }
  }

  return Ok(std::string());
}

AudioFileConcatResult concatAudioFilesWithMiniAudio(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  if (!isMiniaudioOutputPath(outputPath)) {
    return Err("concatAudioFiles supports miniaudio output only for WAV files.");
  }

  for (const auto &inputPath : inputPaths) {
    if (!hasExtension(inputPath, {"wav"})) {
      return Err("concatAudioFiles WAV output requires all input files to use the WAV extension.");
    }
  }

  std::vector<MiniAudioDecoderGuard> inputs;
  auto inputValidationResult = openAndValidateMiniAudioInputs(inputPaths, inputs);
  if (inputValidationResult.is_err()) {
    return inputValidationResult;
  }

  const ma_uint64 bytesPerFrame =
      inputs.front().channels() * ma_get_bytes_per_sample(inputs.front().format());
  auto outputSizeResult = validateRiffWaveOutputSize(inputs, bytesPerFrame);
  if (outputSizeResult.is_err()) {
    return outputSizeResult;
  }

  MiniAudioEncoderGuard output;
  auto outputResult = output.open(
      outputPath, inputs.front().format(), inputs.front().sampleRate(), inputs.front().channels());
  if (outputResult.is_err()) {
    return outputResult;
  }

  std::vector<uint8_t> buffer(miniaudioChunkFrames * bytesPerFrame);
  for (auto &input : inputs) {
    while (true) {
      ma_uint64 framesRead = 0;
      const ma_result result =
          ma_decoder_read_pcm_frames(input.get(), buffer.data(), miniaudioChunkFrames, &framesRead);
      if (result != MA_SUCCESS && result != MA_AT_END) {
        return Err(
            "Failed to decode frames from '" + input.filePath() +
            "' with miniaudio: " + parseMiniAudioError(result));
      }

      if (framesRead == 0) {
        break;
      }

      auto writeResult = output.write(input.filePath(), buffer.data(), framesRead);
      if (writeResult.is_err()) {
        return writeResult;
      }

      if (result == MA_AT_END) {
        break;
      }
    }
  }

  return Ok(outputPath);
}

AudioFileConcatResult concatAudioFilesWithOsRemux(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  for (const auto &inputPath : inputPaths) {
    if (!extensionsCompatibleForRemux(inputPath, outputPath)) {
      return Err(
          "concatAudioFiles remux requires all input files to use the same container family as the output.");
    }
  }

  return remuxConcatAudioFiles(inputPaths, outputPath);
}

} // namespace

std::string normalizeFilePath(const std::string &path) {
  if (path.starts_with(fileUrlPrefix)) {
    return percentDecode(path.substr(std::strlen(fileUrlPrefix)));
  }

  return percentDecode(path);
}

AudioFileConcatResult concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  std::vector<std::string> normalizedInputPaths;
  normalizedInputPaths.reserve(inputPaths.size());
  for (const auto &inputPath : inputPaths) {
    normalizedInputPaths.push_back(normalizeFilePath(inputPath));
  }

  const std::string normalizedOutputPath = normalizeFilePath(outputPath);

  auto pathValidationResult = validatePaths(normalizedInputPaths, normalizedOutputPath);
  if (pathValidationResult.is_err()) {
    return pathValidationResult;
  }

  if (isMiniaudioOutputPath(normalizedOutputPath)) {
    return concatAudioFilesWithMiniAudio(normalizedInputPaths, normalizedOutputPath)
        .map([&outputPath](const std::string &) { return outputPath; });
  }

  if (isOsRemuxOutputPath(normalizedOutputPath)) {
    return concatAudioFilesWithOsRemux(normalizedInputPaths, normalizedOutputPath)
        .map([&outputPath](const std::string &) { return outputPath; });
  }

  return Err("concatAudioFiles supports WAV and M4A/MP4 output.");
}

} // namespace audioapi
