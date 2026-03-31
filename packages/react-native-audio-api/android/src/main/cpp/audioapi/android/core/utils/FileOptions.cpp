#include <android/log.h>
#include <audioapi/android/core/utils/FileOptions.h>
#include <audioapi/android/system/NativeFileInfo.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <string>

namespace audioapi::android::fileoptions {

Result<NoneType, std::string> createDirectoryIfNotExists(const std::string &directoryPath) {
  std::error_code ec;

  if (std::filesystem::exists(directoryPath, ec)) {
    return Result<NoneType, std::string>::Ok(None);
  }

  bool created = std::filesystem::create_directories(directoryPath, ec);

  if (!created) {
    return Result<NoneType, std::string>::Err("Failed to create directory: " + directoryPath);
  }

  if (ec) {
    return Result<NoneType, std::string>::Err(ec.message());
  }

  return Result<NoneType, std::string>::Ok(None);
}

std::string getTimestampString() {
  auto tNow = std::chrono::system_clock::now();
  return std::format("{:%Y%m%d_%H%M%S}", std::chrono::floor<std::chrono::seconds>(tNow));
}

std::string getDirectory(const std::shared_ptr<AudioFileProperties> &properties) {
  switch (properties->directory) {
    case AudioFileProperties::FileDirectory::Document:
      return NativeFileInfo::getFilesDir();
    case AudioFileProperties::FileDirectory::Cache:
      return NativeFileInfo::getCacheDir();
    default:
      return NativeFileInfo::getCacheDir();
  }
}

std::string getFileExtension(const std::shared_ptr<AudioFileProperties> &properties) {
  switch (properties->format) {
    case AudioFileProperties::Format::WAV:
      return "wav";
    case AudioFileProperties::Format::CAF:
      return "caf";
    case AudioFileProperties::Format::M4A:
      return "m4a";
    case AudioFileProperties::Format::FLAC:
      return "flac";
    default:
      return "m4a";
  }
}

Result<std::string, std::string> getFilePath(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileNameOverride) {
  std::string directory = getDirectory(properties);
  std::string subDirectory = std::format("{}/{}", directory, properties->subDirectory);
  std::string fileTimestamp = getTimestampString();
  std::string extension = getFileExtension(properties);

  auto result = createDirectoryIfNotExists(subDirectory);

  if (!result.is_ok()) {
    return Result<std::string, std::string>::Err(result.unwrap_err());
  }

  auto filePath = fileNameOverride.length() > 0
      ? std::format("{}/{}.{}", subDirectory, fileNameOverride, extension)
      : std::format(
            "{}/{}_{}.{}", subDirectory, properties->fileNamePrefix, fileTimestamp, extension);

  return Result<std::string, std::string>::Ok(filePath);
}

} // namespace audioapi::android::fileoptions
