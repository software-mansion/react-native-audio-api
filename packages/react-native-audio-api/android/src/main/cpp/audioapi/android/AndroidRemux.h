#pragma once

#include <audioapi/utils/Result.hpp>

#include <string>
#include <vector>

namespace audioapi::android::remux {

/// Packet-copy remux of compatible AAC-in-M4A/MP4 via AMediaExtractor + AMediaMuxer.
[[nodiscard]] Result<std::string, std::string> concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath);

} // namespace audioapi::android::remux
