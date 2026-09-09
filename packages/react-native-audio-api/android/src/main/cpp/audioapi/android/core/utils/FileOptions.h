#pragma once

#include <audioapi/utils/Result.hpp>
#include <memory>
#include <string>

namespace audioapi {

class AudioFileProperties;

namespace android::fileoptions {

Result<NoneType, std::string> createDirectoryIfNotExists(const std::string &directoryPath);

std::string getDirectory(const std::shared_ptr<AudioFileProperties> &properties);
Result<std::string, std::string> getFilePath(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileName);

} // namespace android::fileoptions

} // namespace audioapi
