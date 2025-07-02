#pragma once

#include <audioapi/utils/AudioArray.h>
#include <audioapi/dsp/FFT.h>
#include <vector>
#include <cstring>
#include <complex>
#include <memory>

namespace audioapi {

class AudioBuffer;

class Convolver {
 public:
    Convolver();
    bool init(size_t blockSize, const audioapi::AudioArray &ir, size_t irLen);
    void process(const AudioArray &input, AudioArray &output, size_t len);
    void reset();
 private:
    size_t _blockSize;
    size_t _segSize;
    size_t _segCount;
    size_t _fftComplexSize;
    std::vector<std::vector<std::complex<float>>> _segments;
    std::vector<std::vector<std::complex<float>>> _segmentsIR;
    AudioArray _fftBuffer;
    std::shared_ptr<audioapi::dsp::FFT> _fft;
    std::vector<std::complex<float>> _preMultiplied;
    size_t _current;
    AudioArray _inputBuffer;
};
} // namespace audioapi
