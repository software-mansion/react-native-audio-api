#pragma once

#include <stdexcept>
#include <string>
#include <algorithm>

namespace audioapi {

    enum class OscillatorType {
        SINE,
        SQUARE,
        SAWTOOTH,
        TRIANGLE,
        CUSTOM
    };

} // namespace audioapi
