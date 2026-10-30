// implementation of linear convolution algorithm described in this paper:
// https://publications.rwth-aachen.de/record/466561/files/466561.pdf page 110

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#include <audioapi/dsp/Convolver.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.hpp>

#include <algorithm>
#include <memory>

namespace audioapi {

Convolver::Convolver()
    : _trueSegmentCount(0),
      _blockSize(0),
      _segSize(0),
      _segCount(0),
      _fftComplexSize(0),
      _fft(nullptr),
      _current(0),
      _inputFill(0) {}

void Convolver::reset() {
  _blockSize = 0;
  _segSize = 0;
  _segCount = 0;
  _fftComplexSize = 0;
  _current = 0;
  _inputFill = 0;
  _fft = nullptr;
  _segments.clear();
  _segmentsIR.clear();
  _preMultiplied.clear();
  _blockSpectrum.clear();
  if (_fftBuffer != nullptr) {
    _fftBuffer->zero();
  }
  if (_inputBuffer != nullptr) {
    _inputBuffer->zero();
  }
}

bool Convolver::init(size_t blockSize, const AudioArray &ir, size_t irLen) {
  reset();
  // blockSize must be a power of two
  if ((blockSize & (blockSize - 1))) {
    return false;
  }

  _blockSize = blockSize;
  _trueSegmentCount =
      static_cast<size_t>((std::ceil(static_cast<float>(irLen) / static_cast<float>(_blockSize))));

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
  _fftBuffer = std::make_unique<DSPAudioArray>(_segSize);

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
      _fftBuffer->copy(ir, i * _blockSize, 0, samplesToCopy);
    }
    // Each sub filter is zero-padded to length 2B and transformed using a
    // 2B-point real-to-complex FFT. A short last sub filter is padded too, or it
    // would keep the previous sub filter's tail.
    _fftBuffer->zero(samplesToCopy, _segSize - samplesToCopy);
    _fft->doFFT(*_fftBuffer, segment);
    _segmentsIR.push_back(segment);
  }

  _preMultiplied = aligned_vec_complex(_fftComplexSize);
  _blockSpectrum = aligned_vec_complex(_fftComplexSize);
  _inputBuffer = std::make_unique<DSPAudioArray>(_segSize);
  _current = 0;
  _inputFill = 0;

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

  // Bin 0 holds two packed real coefficients (pffft layout): DC in the real
  // part and Nyquist in the imaginary part. They multiply independently as
  // reals, not as one complex number.
  pre[0].real(pre[0].real() + ir[0].real() * audio[0].real());
  pre[0].imag(pre[0].imag() + ir[0].imag() * audio[0].imag());

/// @note Using ARM NEON intrinsics for SIMD optimization
/// This implementation is on average 2x faster than the scalar version on ARM
/// architectures With 16-byte alignment it can be even faster up to 2.5x
#ifdef __ARM_NEON
  size_t j = 1;

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

    // new_re = pre_re + ir_re*a_re - ir_im*a_im
#if defined(__aarch64__) || defined(__arm64__)
    float32x4_t new_re = vfmaq_f32(pre_de.val[0], ir_re, a_re);
    new_re = vfmsq_f32(new_re, ir_im, a_im);
    // new_im = pre_im + ir_re*a_im + ir_im*a_re
    float32x4_t new_im = vfmaq_f32(pre_de.val[1], ir_re, a_im);
    new_im = vfmaq_f32(new_im, ir_im, a_re);
#else
    float32x4_t new_re = vmlaq_f32(pre_de.val[0], ir_re, a_re);
    new_re = vmlsq_f32(new_re, ir_im, a_im);
    // new_im = pre_im + ir_re*a_im + ir_im*a_re
    float32x4_t new_im = vmlaq_f32(pre_de.val[1], ir_re, a_im);
    new_im = vmlaq_f32(new_im, ir_im, a_re);
#endif

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
  for (size_t i = 1; i < n; ++i) {
    pre[i] += ir[i] * audio[i];
  }
#endif
}

void Convolver::process(const DSPAudioArray &input, DSPAudioArray &output, size_t framesToProcess) {
  if (_segCount == 0) {
    output.zero(0, framesToProcess);
    return;
  }

  size_t processed = 0;
  while (processed < framesToProcess) {
    if (_inputFill == 0) {
      accumulateCompletedBlocks();
    }

    const size_t chunk = std::min(framesToProcess - processed, _blockSize - _inputFill);
    const size_t windowPosition = _blockSize + _inputFill;

    // The input buffer is a 2B-point window: the previous block on the left, the block being
    // filled on the right. Frames of this block that have not arrived yet are still zero, and an
    // output frame only depends on input frames at or before its own position, so the partial
    // block yields exact output for every frame received so far.
    _inputBuffer->copy(input, processed, windowPosition, chunk);
    _fft->doFFT(*_inputBuffer, _segments[_current]);

    std::ranges::copy(_preMultiplied, _blockSpectrum.begin());
    pairwise_complex_multiply_fast(_segmentsIR[0], _segments[_current], _blockSpectrum);
    // Of the 2B-point IFFT, the left half is discarded (overlap-save); the right half lines up
    // with the right half of the input window.
    _fft->doInverseFFT(_blockSpectrum, *_fftBuffer);
    output.copy(*_fftBuffer, windowPosition, processed, chunk);

    _inputFill += chunk;
    processed += chunk;
    if (_inputFill == _blockSize) {
      advanceBlock();
    }
  }
}

void Convolver::accumulateCompletedBlocks() {
  // Sub filter i meets the input block i blocks back; slot `_current` belongs to the block
  // being filled, so the completed blocks sit at the following FDL slots.
  memset(_preMultiplied.data(), 0, _preMultiplied.size() * sizeof(std::complex<float>));
  for (size_t i = 1; i < _segCount; ++i) {
    const size_t indexAudio = (_current + i) % _segCount;
    pairwise_complex_multiply_fast(_segmentsIR[i], _segments[indexAudio], _preMultiplied);
  }
}

void Convolver::advanceBlock() {
  // The completed block's spectrum stays in its FDL slot; the window slides by one block and
  // the next block starts from silence.
  _inputBuffer->copyWithin(_blockSize, 0, _blockSize);
  _inputBuffer->zero(_blockSize, _blockSize);
  _inputFill = 0;
  _current = (_current > 0) ? _current - 1 : _segCount - 1;
}
} // namespace audioapi
