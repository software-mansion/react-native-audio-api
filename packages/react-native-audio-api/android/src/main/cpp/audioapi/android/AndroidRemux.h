#pragma once

#include <audioapi/utils/Result.hpp>

#include <string>
#include <vector>

namespace audioapi::android_remux {

using AndroidRemuxResult = Result<std::string, std::string>;

/// Packet-copy remux of compatible AAC-in-M4A/MP4 via AMediaExtractor + AMediaMuxer.
[[nodiscard]] AndroidRemuxResult concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath);

} // namespace audioapi::android_remux
