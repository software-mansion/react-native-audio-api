#pragma once

#include <audioapi/utils/Result.hpp>

#include <string>
#include <vector>

namespace audioapi::ios_remux {

using IOSRemuxResult = Result<std::string, std::string>;

/// Joins AAC-in-M4A/MP4 via AVMutableComposition (re-encodes; passthrough fails
/// on rotated recorder segments).
[[nodiscard]] IOSRemuxResult concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath);

} // namespace audioapi::ios_remux
