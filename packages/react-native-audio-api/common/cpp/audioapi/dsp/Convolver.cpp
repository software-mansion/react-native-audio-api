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
#include <optional>
#include <vector>

namespace audioapi {

Convolver::Convolver()
    : _trueSegmentCount(0),
      _blockSize(0),
      _segSize(0),
      _headSegCount(0),
      _fftComplexSize(0),
      _fft(nullptr),
      _tailSegCount(0) {}

void Convolver::reset() {
  _blockSize = 0;
  _segSize = 0;
  _headSegCount = 0;
  _fftComplexSize = 0;
  _fft = nullptr;
  _segments.clear();
  _segmentsIR.clear();
  _preMultiplied.clear();
  _blocksProcessed = 0;
  if (_fftBuffer != nullptr) {
    _fftBuffer->zero();
  }
  if (_inputBuffer != nullptr) {
    _inputBuffer->zero();
  }

  _tailSegCount = 0;
  _tailSegments.clear();
  _tailSegmentsIR.clear();
  _tailFft = nullptr;
  _tailInputBuffer = nullptr;
  _tailPreMultiplied.clear();
  _tailOutput = nullptr;
  _tailSlots.clear();
  _tailPosition = 0;
  _completedTailBlockPending = false;
  _tailBlocksCompleted.store(0, std::memory_order_relaxed);
  _tailBlocksRendered.store(0, std::memory_order_relaxed);
  _tailMisses = 0;
}

bool Convolver::init(size_t blockSize, const AudioArray &ir, size_t irLen) {
  reset();
  // blockSize must be a power of two
  if (blockSize == 0 || (blockSize & (blockSize - 1))) {
    return false;
  }

  _blockSize = blockSize;
  _trueSegmentCount = (irLen + _blockSize - 1) / _blockSize;

  if (irLen == 0) {
    return true;
  }

  // A tail block must be a whole number of blocks, which holds for powers of two up to its size.
  const bool splitsTail = irLen > HEAD_FRAMES && _blockSize <= TAIL_BLOCK_FRAMES;
  const size_t headLen = splitsTail ? HEAD_FRAMES : irLen;

  // The length-N head is split into P = N/B length-B sub filters
  _headSegCount = (headLen + _blockSize - 1) / _blockSize;
  _segSize = 2 * _blockSize;
  // size of the FFT is 2B, so the complex size is B+1, due to the
  // complex-conjugate symmetricity
  _fftComplexSize = _segSize / 2 + 1;
  _fft = std::make_shared<dsp::FFT>(static_cast<int>(_segSize));
  _fftBuffer = std::make_unique<DSPAudioArray>(_segSize);

  // segments preparation
  _segments.assign(
      _headSegCount, aligned_vec_complex(_fftComplexSize, std::complex<float>(0.0f, 0.0f)));

  // ir preparation
  for (size_t i = 0; i < _headSegCount; ++i) {
    aligned_vec_complex segment(_fftComplexSize);
    const size_t remainingSamples = headLen - (i * _blockSize);
    const size_t samplesToCopy = std::min(_blockSize, remainingSamples);

    // Each sub filter is zero-padded to length 2B and transformed using a
    // 2B-point real-to-complex FFT. The whole buffer is cleared so a final,
    // partial sub filter does not keep samples of the previous one.
    _fftBuffer->zero();
    _fftBuffer->copy(ir, i * _blockSize, 0, samplesToCopy);
    _fft->doFFT(*_fftBuffer, segment);
    _segmentsIR.push_back(segment);
  }

  _preMultiplied = aligned_vec_complex(_fftComplexSize);
  _inputBuffer = std::make_unique<DSPAudioArray>(_segSize);

  if (splitsTail) {
    initTail(ir, irLen);
  }

  return true;
}

void Convolver::initTail(const AudioArray &ir, size_t irLen) {
  const size_t tailLen = irLen - HEAD_FRAMES;
  const size_t tailSegSize = 2 * TAIL_BLOCK_FRAMES;
  const size_t tailFftComplexSize = tailSegSize / 2 + 1;
  const aligned_vec_complex silentSpectrum(tailFftComplexSize, std::complex<float>(0.0f, 0.0f));

  _tailSegCount = (tailLen + TAIL_BLOCK_FRAMES - 1) / TAIL_BLOCK_FRAMES;
  _tailFft = std::make_unique<dsp::FFT>(static_cast<int>(tailSegSize));
  _tailInputBuffer = std::make_unique<DSPAudioArray>(tailSegSize);
  _tailOutput = std::make_unique<DSPAudioArray>(tailSegSize);
  _tailPreMultiplied = silentSpectrum;
  _tailSegments.assign(_tailSegCount + SPARE_TAIL_SEGMENTS, silentSpectrum);

  // _tailInputBuffer is still silent here, so it doubles as the zero-padded sub filter.
  for (size_t i = 0; i < _tailSegCount; ++i) {
    const size_t segmentStart = i * TAIL_BLOCK_FRAMES;
    const size_t samplesToCopy = std::min(TAIL_BLOCK_FRAMES, tailLen - segmentStart);

    _tailInputBuffer->zero();
    _tailInputBuffer->copy(ir, HEAD_FRAMES + segmentStart, 0, samplesToCopy);
    _tailSegmentsIR.push_back(silentSpectrum);
    _tailFft->doFFT(*_tailInputBuffer, _tailSegmentsIR.back());
  }
  _tailInputBuffer->zero();

  _tailSlots = std::vector<TailSlot>(TAIL_SLOT_COUNT);
  for (auto &slot : _tailSlots) {
    slot.spectrum = silentSpectrum;
    slot.frames = std::make_unique<DSPAudioArray>(tailSegSize);
    slot.fft = std::make_unique<dsp::FFT>(static_cast<int>(tailSegSize));
  }
}

const Convolver::aligned_vec_complex &Convolver::inputSpectrum(size_t block, size_t age) const {
  const size_t ringSize = _segments.size();
  return _segments[(block % ringSize + ringSize - age) % ringSize];
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

const Convolver::aligned_vec_complex &Convolver::tailInputSpectrum(size_t block, size_t age) const {
  const size_t ringSize = _tailSegments.size();
  return _tailSegments[(block % ringSize + ringSize - age) % ringSize];
}

void Convolver::accumulateTail(size_t block, aligned_vec_complex &into) const {
  // Input blocks from before the stream started are silent.
  const size_t segments = std::min(_tailSegCount, block + 1);
  for (size_t age = 0; age < segments; ++age) {
    pairwise_complex_multiply_fast(_tailSegmentsIR[age], tailInputSpectrum(block, age), into);
  }
}

void Convolver::process(const DSPAudioArray &input, DSPAudioArray &output) {
  if (_headSegCount == 0) {
    output.zero();
    return;
  }
  const size_t block = _blocksProcessed++;

  // The input buffer acts as a 2B-point sliding window of the input signal.
  // With each new input block, the right half of the input buffer is shifted
  // to the left and the new block is stored in the right half.
  _inputBuffer->copyWithin(_blockSize, 0, _blockSize);
  _inputBuffer->copy(input, 0, _blockSize, _blockSize);

  // A 2B-point real-to-complex FFT is computed from the input buffer,
  // resulting in B+1 complex-conjugate symmetric DFT coefficients. The
  // result is stored in the delay line slot for this block.
  _fft->doFFT(*_inputBuffer, _segments[block % _segments.size()]);

  // The P sub filter spectra are pairwisely multiplied with the input spectra
  // in the FDL. The results are accumulated in the frequency-domain.
  memset(_preMultiplied.data(), 0, _preMultiplied.size() * sizeof(std::complex<float>));
  for (size_t age = 0; age < _headSegCount; ++age) {
    pairwise_complex_multiply_fast(_segmentsIR[age], inputSpectrum(block, age), _preMultiplied);
  }

  // Of the accumulated spectral convolutions, an 2B-point complex-to-real
  // IFFT is computed. From the resulting 2B samples, the left half is
  // discarded and the right half is returned as the next output block.
  _fft->doInverseFFT(_preMultiplied, *_fftBuffer);

  output.copy(*_fftBuffer, _blockSize, 0, _blockSize);

  if (hasTail()) {
    renderTail(output);
  }
}

void Convolver::renderTail(DSPAudioArray &output) {
  if (_tailPosition == 0) {
    const size_t completed = _tailBlocksCompleted.load(std::memory_order_relaxed);
    if (completed >= TAIL_DELAY_BLOCKS) {
      loadTailBlock(completed - TAIL_DELAY_BLOCKS);
    }
  }
  output.sum(*_tailOutput, TAIL_BLOCK_FRAMES + _tailPosition, 0, _blockSize);

  // Read from the head's window, not from `input`, which may alias `output`.
  _tailInputBuffer->copy(*_inputBuffer, _blockSize, TAIL_BLOCK_FRAMES + _tailPosition, _blockSize);
  _tailPosition += _blockSize;
  if (_tailPosition == TAIL_BLOCK_FRAMES) {
    _tailPosition = 0;
    completeTailInputBlock();
  }
}

void Convolver::loadTailBlock(size_t block) {
  const TailSlot &slot = _tailSlots[block % TAIL_SLOT_COUNT];
  if (slot.state.load(std::memory_order_acquire) == readyState(block)) {
    _tailOutput->copy(*slot.frames, TAIL_BLOCK_FRAMES, TAIL_BLOCK_FRAMES, TAIL_BLOCK_FRAMES);
  } else {
    ++_tailMisses;
    memset(_tailPreMultiplied.data(), 0, _tailPreMultiplied.size() * sizeof(std::complex<float>));
    accumulateTail(block, _tailPreMultiplied);
    _tailFft->doInverseFFT(_tailPreMultiplied, *_tailOutput);
  }
  _tailBlocksRendered.store(block + 1, std::memory_order_release);
}

void Convolver::completeTailInputBlock() {
  const size_t block = _tailBlocksCompleted.load(std::memory_order_relaxed);
  _tailFft->doFFT(*_tailInputBuffer, _tailSegments[block % _tailSegments.size()]);
  _tailBlocksCompleted.store(block + 1, std::memory_order_release);
  _tailInputBuffer->copyWithin(TAIL_BLOCK_FRAMES, 0, TAIL_BLOCK_FRAMES);
  _completedTailBlockPending = true;
}

std::optional<size_t> Convolver::takeCompletedTailBlock() {
  if (!_completedTailBlockPending) {
    return std::nullopt;
  }
  _completedTailBlockPending = false;
  return _tailBlocksCompleted.load(std::memory_order_relaxed) - 1;
}

void Convolver::computeTail(size_t block) {
  if (!hasTail() || block >= _tailBlocksCompleted.load(std::memory_order_acquire) ||
      block < _tailBlocksRendered.load(std::memory_order_acquire)) {
    return;
  }
  TailSlot &slot = _tailSlots[block % TAIL_SLOT_COUNT];
  size_t state = slot.state.load(std::memory_order_acquire);
  if ((state & BUSY_BIT) != 0 || state >= readyState(block)) {
    return; // another worker is writing, or the slot already holds this block or a newer one
  }

  // Claim the slot only if it still holds the state checked above, so a worker preempted
  // between the check and the claim can never take a slot a newer block has claimed since.
  if (!slot.state.compare_exchange_strong(
          state, readyState(block) | BUSY_BIT, std::memory_order_acq_rel)) {
    return; // the slot changed since the check; the audio thread will compute this tail inline
  }
  memset(slot.spectrum.data(), 0, slot.spectrum.size() * sizeof(std::complex<float>));
  accumulateTail(block, slot.spectrum);
  slot.fft->doInverseFFT(slot.spectrum, *slot.frames);
  slot.state.store(readyState(block), std::memory_order_release);
}
} // namespace audioapi
