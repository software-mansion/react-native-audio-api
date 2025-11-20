// implementation of linear convolution algorithm described in this paper:
// https://publications.rwth-aachen.de/record/466561/files/466561.pdf page 110

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/dsp/Convolver.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>

namespace audioapi {

Convolver::Convolver()
    : _blockSize(0),
      _segSize(0),
      _segCount(0),
      _fftComplexSize(0),
      _segments(),
      _segmentsIR(),
      _fftBuffer(0),
      _fft(nullptr),
      _preMultiplied(),
      _current(0),
      _inputBuffer(0) {}

void Convolver::reset() {
  _blockSize = 0;
  _segSize = 0;
  _segCount = 0;
  _fftComplexSize = 0;
  _current = 0;
  _fft = nullptr;
  _segments.clear();
  _segmentsIR.clear();
  _preMultiplied.clear();
  _fftBuffer.zero();
  _inputBuffer.zero();
}

bool Convolver::init(size_t blockSize, const audioapi::AudioArray &ir, size_t irLen) {
  reset();
  // blockSize must be a power of two
  if ((blockSize & (blockSize - 1))) {
    return false;
  }

  // Ignore zeros at the end of the impulse response because they only waste
  // computation time
  _blockSize = blockSize;
  _trueSegmentCount =
      static_cast<size_t>((std::ceil(static_cast<float>(irLen) / static_cast<float>(_blockSize))));
  while (irLen > 0 && ::fabs(ir[irLen - 1]) < 10e-3) {
    --irLen;
  }

  if (irLen == 0) {
    return true;
  }

  // The length-N is split into P = N/B length-B sub filters
  _segCount =
      static_cast<size_t>((std::ceil(static_cast<float>(irLen) / static_cast<float>(_blockSize))));
  _segSize = 2 * _blockSize;
  // size of the FFT is 2B, so the complex size is B+1, due to the
  // complex-conjugate symmetricity
  _fftComplexSize = _segSize / 2 + 1;
  _fft = std::make_shared<dsp::FFT>(static_cast<int>(_segSize));
  _fftBuffer.resize(_segSize);

  // segments preparation
  for (int i = 0; i < _segCount; ++i) {
    aligned_vec_complex vec(_fftComplexSize, std::complex<float>(0.0f, 0.0f));
    _segments.push_back(vec);
  }

  // ir preparation
  for (int i = 0; i < _segCount; ++i) {
    aligned_vec_complex segment(_fftComplexSize);
    const size_t remainingSamples = irLen - (i * _blockSize);
    const size_t samplesToCopy = std::min(_blockSize, remainingSamples);

    if (samplesToCopy > 0) {
      memcpy(_fftBuffer.getData(), ir.getData() + i * _blockSize, samplesToCopy * sizeof(float));
    }
    // Each sub filter is zero-padded to length 2B and transformed using a
    // 2B-point real-to-complex FFT.
    memset(_fftBuffer.getData() + _blockSize, 0, _blockSize * sizeof(float));
    _fft->doFFT(_fftBuffer.getData(), segment);
    segment.at(0).imag(0.0f); // ensure DC component is real
    _segmentsIR.push_back(segment);
  }

  _preMultiplied = aligned_vec_complex(_fftComplexSize);
  _inputBuffer.resize(_segSize);
  _current = 0;

  return true;
}

/// @brief Fast pairwise complex multiplication using ARM NEON intrinsics
/// @param ir Impulse response
/// @param audio Input audio signal
/// @param pre Output buffer for pre-multiplied results
/// @note IMPORTANT: ir, audio, and pre must be the same size and should be
/// aligned to 16 bytes for optimal performance
void pairwise_complex_multiply_fast(
    const Convolver::aligned_vec_complex &ir,
    const Convolver::aligned_vec_complex &audio,
    Convolver::aligned_vec_complex &pre) {
  size_t n = ir.size();

/// @note Using ARM NEON intrinsics for SIMD optimization
/// This implementation is on average 2x faster than the scalar version on ARM
/// architectures With 16-byte alignment it can be even faster up to 2.5x
#ifdef __ARM_NEON
  size_t j = 0;

  // Main vector loop: process 4 complex samples (8 floats) per iteration using
  // vld2q/vst2q deinterleave
  for (; j <= n - 4; j += 4) {
    // load de-interleaved real/imag for 4 complex values
    float32x4x2_t ir_de = vld2q_f32(reinterpret_cast<const float *>(&ir[j]));
    float32x4x2_t a_de = vld2q_f32(reinterpret_cast<const float *>(&audio[j]));
    float32x4x2_t pre_de = vld2q_f32(reinterpret_cast<float *>(&pre[j]));

    float32x4_t ir_re = ir_de.val[0];
    float32x4_t ir_im = ir_de.val[1];
    float32x4_t a_re = a_de.val[0];
    float32x4_t a_im = a_de.val[1];

    // real = ir_re * a_re - ir_im * a_im
    float32x4_t real = vmulq_f32(ir_re, a_re);
    real = vmlsq_f32(real, ir_im, a_im);
    // imag = ir_re * a_im + ir_im * a_re
    float32x4_t imag = vmulq_f32(ir_re, a_im);
    imag = vmlaq_f32(imag, ir_im, a_re);

    // accumulate into pre
    float32x4_t new_re = vaddq_f32(pre_de.val[0], real);
    float32x4_t new_im = vaddq_f32(pre_de.val[1], imag);

    float32x4x2_t out_de;
    out_de.val[0] = new_re;
    out_de.val[1] = new_im;

    vst2q_f32(reinterpret_cast<float *>(&pre[j]), out_de);
  }

  // Tail
  for (; j < n; ++j) {
    pre[j] += ir[j] * audio[j];
  }

#else
  // Fallback scalar implementation
  for (size_t i = 0; i < n; ++i) {
    pre[i] += ir[i] * audio[i];
  }
#endif
}

void Convolver::process(float *data, float *outputData) {
  // The input buffer acts as a 2B-point sliding window of the input signal.
  // With each new input block, the right half of the input buffer is shifted
  // to the left and the new block is stored in the right half.
  memmove(_inputBuffer.getData(), _inputBuffer.getData() + _blockSize, _blockSize * sizeof(float));
  memcpy(_inputBuffer.getData() + _blockSize, data, _blockSize * sizeof(float));

  // All contents (DFT spectra) in the FDL are shifted up by one slot.
  _current = (_current > 0) ? _current - 1 : _segCount - 1;
  // A 2B-point real-to-complex FFT is computed from the input buffer,
  // resulting in B+1 complex-conjugate symmetric DFT coefficients. The
  // result is stored in the first FDL slot.
  // _current marks first FDL slot, which is the current input block.
  _fft->doFFT(_inputBuffer.getData(), _segments[_current]);
  _segments[_current][0].imag(0.0f); // ensure DC component is real

  // The P sub filter spectra are pairwisely multiplied with the input spectra
  // in the FDL. The results are accumulated in the frequency-domain.
  memset(_preMultiplied.data(), 0, _preMultiplied.size() * sizeof(std::complex<float>));
  // this is a bottleneck of the algorithm
  for (int i = 0; i < _segCount; ++i) {
    const int indexAudio = (_current + i) % _segCount;
    const auto &impulseResponseSegment = _segmentsIR[i];
    const auto &audioSegment = _segments[indexAudio];
    pairwise_complex_multiply_fast(impulseResponseSegment, audioSegment, _preMultiplied);
  }
  // Of the accumulated spectral convolutions, an 2B-point complex-to-real
  // IFFT is computed. From the resulting 2B samples, the left half is
  // discarded and the right half is returned as the next output block.
  _fft->doInverseFFT(_preMultiplied, _fftBuffer.getData());

  memcpy(outputData, _fftBuffer.getData() + _blockSize, _blockSize * sizeof(float));
}
} // namespace audioapi
