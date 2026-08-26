#pragma once

#include <cstdint>

namespace audioapi {

enum class PanningModelType : std::uint8_t { EqualPower, HRTF };

enum class DistanceModelType : std::uint8_t { Inverse, Linear, Exponential };

} // namespace audioapi
