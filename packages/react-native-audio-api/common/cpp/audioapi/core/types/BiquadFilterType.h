#pragma once

namespace audioapi {

enum class BiquadFilterType {
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
