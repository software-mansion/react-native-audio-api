#pragma once

#include <string>
#include <memory>
#include <audioapi/utils/Result.hpp>

namespace audioapi {

class AudioFileProperties;

namespace android::fileoptions {

Result<NoneType, std::string> createDirectoryIfNotExists(const std::string &directoryPath);
std::string getTimestampString();

std::string getDirectory(const std::shared_ptr<AudioFileProperties> &properties);
std::string getFileExtension(const std::shared_ptr<AudioFileProperties> &properties);
Result<std::string, std::string> getFilePath(const std::shared_ptr<AudioFileProperties> &properties, const std::string &fileNameOverride);

} // namespace android::fileoptions

} // namespace audioapi
