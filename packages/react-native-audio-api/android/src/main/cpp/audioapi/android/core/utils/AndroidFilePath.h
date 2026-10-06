#pragma once

#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>

namespace audioapi {

class AudioFileProperties;

namespace android::file_path {

using ResolveFilePathResult = Result<std::string, std::string>;

/// Resolves the absolute output path for a recording, creating the target directory.
[[nodiscard]] ResolveFilePathResult resolveFilePath(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileName);

} // namespace android::file_path

} // namespace audioapi
