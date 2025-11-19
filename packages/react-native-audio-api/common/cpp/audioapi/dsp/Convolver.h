#pragma once

#include <audioapi/dsp/FFT.h>
#include <audioapi/utils/AlignedAllocator.hpp>
#include <audioapi/utils/AudioArray.h>
#include <complex>
#include <cstring>
#include <memory>
#include <vector>

namespace audioapi {

class AudioBuffer;

class Convolver {
  using aligned_vec_complex =
      std::vector<std::complex<float>, AlignedAllocator<std::complex<float>, 16>>;

 public:
  Convolver();
  bool init(size_t blockSize, const AudioArray &ir, size_t irLen);
  void process(float *inputData, float *outputData);
  void reset();
  inline size_t getSegCount() const {
    return _trueSegmentCount;
  }

 private:
  size_t _trueSegmentCount;
  size_t _blockSize;
  size_t _segSize;
  size_t _segCount;
  size_t _fftComplexSize;
  std::vector<aligned_vec_complex> _segments;
  std::vector<aligned_vec_complex> _segmentsIR;
  AudioArray _fftBuffer;
  std::shared_ptr<dsp::FFT> _fft;
  aligned_vec_complex _preMultiplied;
  size_t _current;
  AudioArray _inputBuffer;

  friend void pairwise_complex_multiply_fast(
      const aligned_vec_complex &ir,
      const aligned_vec_complex &audio,
      aligned_vec_complex &pre);
};
} // namespace audioapi
