#pragma once

#include <cstdint>

namespace audioapi {

enum class AudioFormat : uint8_t { UNKNOWN, WAV, OGG, FLAC, AAC, MP3, M4A, MP4, MOV };
} // namespace audioapi
