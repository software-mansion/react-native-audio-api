#include <audioapi/dsp/Windows.h>

namespace audioapi::Windows {

void Hann::apply(float *data, int length) const {
    for (int i = 0; i < length; ++i) {
        auto x = static_cast<float>(i) / static_cast<float>(length - 1);
        auto window = 0.5f - 0.5f * cos(2 * PI * x);
        data[i] *= window * amplitude_;
    }
}

void Blackman::apply(float *data, int length) const {
    for (int i = 0; i < length; ++i) {
        auto x = static_cast<float>(i) / static_cast<float>(length - 1);
        auto window = 0.42f - 0.5f * cos(2 * PI * x) + 0.08f * cos(4 * PI * x);
        data[i] *= window * amplitude_;
    }
}

}
