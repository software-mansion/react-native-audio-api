#include <audioapi/utils/AudioFileProperties.h>

#include <audioapi/core/utils/RecordingFileName.h>
#include <audioapi/jsi/HostObject.h>
#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AudioFileProperties::AudioFileProperties(
    PathConfig path,
    AudioLayout fileLayout,
    EncodingConfig encoding,
    WriterConfig writer)
    : path(std::move(path)), fileLayout(fileLayout), encoding(encoding), writer(writer) {}

namespace {

constexpr int MAX_FLAC_COMPRESSION_LEVEL = 8;

/// The JS layer passes enums as plain numbers, so any value can arrive in one.
template <typename Enum>
bool isWithin(Enum value, Enum last) {
  return static_cast<std::uint8_t>(value) <= static_cast<std::uint8_t>(last);
}

/// A positive sample rate, and between 1 and MAX_FILE_CHANNEL_COUNT channels.
Result<NoneType, std::string> validateFileLayout(const AudioLayout &fileLayout) {
  if (!(fileLayout.sampleRate > 0)) {
    return Err("sampleRate must be greater than 0.");
  }
  if (fileLayout.channelCount <= 0 ||
      fileLayout.channelCount > AudioFileProperties::MAX_FILE_CHANNEL_COUNT) {
    return Err("channelCount must be 1 (mono) or 2 (stereo); file output supports no more.");
  }
  return Ok(None);
}

} // namespace

Result<NoneType, std::string> AudioFileProperties::PathConfig::validate() const {
  auto fileNameResult = recording_file_name::validateFileName(fileName);
  if (fileNameResult.is_err()) {
    return fileNameResult;
  }
  if (!isWithin(directory, FileDirectory::Cache)) {
    return Err("directory is not a FileDirectory value.");
  }
  return Ok(None);
}

Result<NoneType, std::string> AudioFileProperties::EncodingConfig::validate() const {
  if (!isWithin(format, FileFormat::ALAW)) {
    return Err("format is not a FileFormat value.");
  }
  if (!isWithin(bitDepth, BitDepth::Bit32)) {
    return Err("bitDepth is not a BitDepth value.");
  }
  if (!isWithin(iosAudioQuality, IOSAudioQuality::Max)) {
    return Err("iosQuality is not an IOSAudioQuality value.");
  }
  if (flacCompressionLevel < 0 || flacCompressionLevel > MAX_FLAC_COMPRESSION_LEVEL) {
    return Err(
        "flacCompressionLevel must be between 0 and " + std::to_string(MAX_FLAC_COMPRESSION_LEVEL) +
        ".");
  }
  return Ok(None);
}

Result<NoneType, std::string> AudioFileProperties::validate() const {
  return path.validate()
      .and_then([this](NoneType) { return validateFileLayout(fileLayout); })
      .and_then([this](NoneType) { return encoding.validate(); });
}

std::shared_ptr<AudioFileProperties> AudioFileProperties::CreateFromJSIValue(
    facebook::jsi::Runtime &runtime,
    const facebook::jsi::Value &value) {
  auto options = value.getObject(runtime);

  FileDirectory directory =
      static_cast<FileDirectory>(options.getProperty(runtime, "directory").getNumber());

  std::string subDirectory =
      options.getProperty(runtime, "subDirectory").asString(runtime).utf8(runtime);

  std::string fileName = options.getProperty(runtime, "fileName").asString(runtime).utf8(runtime);

  int channelCount = static_cast<int>(options.getProperty(runtime, "channelCount").getNumber());

  size_t rotateIntervalBytes =
      static_cast<size_t>(options.getProperty(runtime, "rotateIntervalBytes").getNumber());

  FileFormat format = static_cast<FileFormat>(options.getProperty(runtime, "format").getNumber());

  int androidFlushIntervalMs =
      static_cast<int>(options.getProperty(runtime, "androidFlushIntervalMs").getNumber());

  auto presetOptions = options.getProperty(runtime, "preset").getObject(runtime);

  float sampleRate =
      static_cast<float>(presetOptions.getProperty(runtime, "sampleRate").getNumber());

  size_t bitRate = static_cast<size_t>(presetOptions.getProperty(runtime, "bitRate").getNumber());

  BitDepth bitDepth =
      static_cast<BitDepth>(presetOptions.getProperty(runtime, "bitDepth").getNumber());

  int flacCompressionLevel =
      static_cast<int>(presetOptions.getProperty(runtime, "flacCompressionLevel").getNumber());

  IOSAudioQuality iosAudioQuality =
      static_cast<IOSAudioQuality>(presetOptions.getProperty(runtime, "iosQuality").getNumber());

  return std::make_shared<AudioFileProperties>(
      PathConfig{
          .directory = directory,
          .subDirectory = std::move(subDirectory),
          .fileName = std::move(fileName),
      },
      AudioLayout{
          .sampleRate = sampleRate,
          .channelCount = channelCount,
      },
      EncodingConfig{
          .format = format,
          .bitRate = bitRate,
          .bitDepth = bitDepth,
          .flacCompressionLevel = flacCompressionLevel,
          .iosAudioQuality = iosAudioQuality,
      },
      WriterConfig{
          .rotateIntervalBytes = rotateIntervalBytes,
          .androidFlushIntervalMs = androidFlushIntervalMs,
      });
}

} // namespace audioapi
