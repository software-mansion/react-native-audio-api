#pragma once

#include <audioapi/core/Constants.h>

#include <cmath>
#include <algorithm>

namespace audioapi::Windows {

//https://en.wikipedia.org/wiki/Hann_function
// https://www.sciencedirect.com/topics/engineering/hanning-window
class Hann {
public:
    static Hann create(float amplitude) {
        return Hann(amplitude);
    }

    float operator()(float x) const {
        return amplitude_ * (0.5f - 0.5f * std::cos(2 * PI * x));
    }

private:
    explicit Hann(float amplitude) : amplitude_(amplitude) {}

    // 1/L = amplitude
    float amplitude_;

};
} // namespace audioapi::Windows
