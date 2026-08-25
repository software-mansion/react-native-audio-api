#pragma once

#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>

namespace audioapi {

class AudioFileProperties;

namespace ios_filepath {

using ResolveFilePathResult = Result<std::string, std::string>;

/// Resolves the absolute output path for a recording, creating the target directory.
/// Pure-C++ wrapper over ios::fileoptions::getFileURL, whose header cannot be included
/// from common code because it names Objective-C types unconditionally.
[[nodiscard]] ResolveFilePathResult resolveFilePath(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileNameOverride);

} // namespace ios_filepath

} // namespace audioapi
