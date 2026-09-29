#include <audioapi/utils/AudioFileProperties.h>

#include <audioapi/jsi/HostObject.h>
#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AudioFileProperties::AudioFileProperties(
    PathConfig path,
    StreamConfig stream,
    EncodingConfig encoding,
    WriterConfig writer)
    : path(std::move(path)), stream(stream), encoding(encoding), writer(writer) {}

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

  Format format = static_cast<Format>(options.getProperty(runtime, "format").getNumber());

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
      StreamConfig{
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
