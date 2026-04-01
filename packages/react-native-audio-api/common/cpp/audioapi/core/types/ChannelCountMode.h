#pragma once

#include <cstdint>

namespace audioapi {

enum class ChannelCountMode : std::uint8_t { MAX, CLAMPED_MAX, EXPLICIT };

}
