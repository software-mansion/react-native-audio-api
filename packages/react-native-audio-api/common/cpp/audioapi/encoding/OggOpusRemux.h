#pragma once

#include <audioapi/utils/Result.hpp>

#include <string>
#include <vector>

#if !RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED

namespace audioapi::ogg_opus {

/// Joins Ogg/Opus files into one logical stream without re-encoding: the first input's
/// OpusHead and OpusTags open the output, and every audio packet is re-paged with its granule
/// position shifted by the audio before it. Inputs must share the first one's channel count.
///
/// Only the end of the whole stream can be trimmed, so the silence an input's last packet was
/// padded with, and the lookahead the next input's first packets carry, stay audible as a few
/// milliseconds at each seam, as with the M4A remux. Ok carries @p outputPath.
[[nodiscard]] Result<std::string, std::string> concatOggOpusFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath);

} // namespace audioapi::ogg_opus

#endif // !RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED
