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
    bool init(size_t blockSize, const AudioArray &ir, size_t irLen);
    void process(float* inputData, float* outputData);
    void reset();
    inline size_t getSegCount() const { return _trueSegmentCount; }
 private:
    size_t _trueSegmentCount;
    size_t _blockSize;
    size_t _segSize;
    size_t _segCount;
    size_t _fftComplexSize;
    std::vector<std::vector<std::complex<float>>> _segments;
    std::vector<std::vector<std::complex<float>>> _segmentsIR;
    AudioArray _fftBuffer;
    std::shared_ptr<dsp::FFT> _fft;
    std::vector<std::complex<float>> _preMultiplied;
    size_t _current;
    AudioArray _inputBuffer;
};
} // namespace audioapi
