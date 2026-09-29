#include <android/log.h>
#include <audioapi/android/core/utils/FileOptions.h>
#include <audioapi/android/system/NativeFileInfo.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <filesystem>
#include <format>
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

std::string getDirectory(const std::shared_ptr<AudioFileProperties> &properties) {
  if (!properties || properties->path.directory == AudioFileProperties::FileDirectory::Cache) {
    return NativeFileInfo::getCacheDir();
  }
  return NativeFileInfo::getFilesDir();
}

Result<std::string, std::string> getFilePath(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileName) {
  std::string directory = getDirectory(properties);
  std::string subDirectory = std::format("{}/{}", directory, properties->path.subDirectory);

  auto result = createDirectoryIfNotExists(subDirectory);

  if (!result.is_ok()) {
    return Result<std::string, std::string>::Err(result.unwrap_err());
  }

  return Result<std::string, std::string>::Ok(std::format("{}/{}", subDirectory, fileName));
}

} // namespace audioapi::android::fileoptions
