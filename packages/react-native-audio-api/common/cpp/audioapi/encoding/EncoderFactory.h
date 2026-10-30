#pragma once

#include <audioapi/encoding/AudioEncoder.h>

namespace audioapi::encoding {

/// Selects the encoder for the format in @p settings: the bundled Ogg/Opus encoder for Opus in
/// Ogg when the build has the static external libs, the platform's system encoder otherwise.
[[nodiscard]] CreateEncoderResult createEncoder(const EncoderSettings &settings);

} // namespace audioapi::encoding
