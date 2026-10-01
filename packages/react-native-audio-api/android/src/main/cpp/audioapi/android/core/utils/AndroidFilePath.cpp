#include <android/log.h>
#include <audioapi/android/core/utils/AndroidFilePath.h>
#include <audioapi/android/system/NativeFileInfo.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <filesystem>
#include <format>
#include <memory>
#include <string>

namespace audioapi::android::file_path {

namespace {

Result<NoneType, std::string> createDirectoryIfNotExists(const std::string &directoryPath) {
  std::error_code ec;

  if (std::filesystem::exists(directoryPath, ec)) {
    return Ok(None);
  }

  bool created = std::filesystem::create_directories(directoryPath, ec);

  if (!created) {
    return Err("Failed to create directory: " + directoryPath);
  }

  if (ec) {
    return Err(ec.message());
  }

  return Ok(None);
}

std::string getDirectory(const std::shared_ptr<AudioFileProperties> &properties) {
  if (!properties || properties->path.directory == AudioFileProperties::FileDirectory::Cache) {
    return NativeFileInfo::getCacheDir();
  }
  return NativeFileInfo::getFilesDir();
}

} // namespace

ResolveFilePathResult resolveFilePath(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileName) {
  std::string directory = getDirectory(properties);
  std::string subDirectory = std::format("{}/{}", directory, properties->path.subDirectory);

  auto result = createDirectoryIfNotExists(subDirectory);

  if (!result.is_ok()) {
    return Err(result.unwrap_err());
  }

  return Ok(std::format("{}/{}", subDirectory, fileName));
}

} // namespace audioapi::android::file_path
