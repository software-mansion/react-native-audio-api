#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>

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

}
