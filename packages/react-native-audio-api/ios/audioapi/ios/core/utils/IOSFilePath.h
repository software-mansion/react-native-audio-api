#pragma once

#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>

namespace audioapi {

class AudioFileProperties;

namespace ios_filepath {

using ResolveFilePathResult = Result<std::string, std::string>;

/// Resolves the absolute output path for a recording, creating the target directory.
[[nodiscard]] ResolveFilePathResult resolveFilePath(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileName);

} // namespace ios_filepath

} // namespace audioapi
