#pragma once

#include <stdexcept>
#include <string>
#include <algorithm>

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
