#pragma once

#include <cstdint>

namespace audioapi {

enum class BiquadFilterType : std::uint8_t {
  LOWPASS,
  HIGHPASS,
  BANDPASS,
  LOWSHELF,
  HIGHSHELF,
  PEAKING,
  NOTCH,
  ALLPASS
};
} // namespace audioapi
