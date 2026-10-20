#pragma once

#include <audioapi/dsp/FFT.h>
#include <audioapi/utils/AlignedAllocator.hpp>
#include <audioapi/utils/AudioArray.hpp>
#include <atomic>
#include <complex>
#include <cstring>
#include <memory>
#include <vector>

namespace audioapi {

/// @brief Uniformly partitioned FFT convolver whose impulse response is split into a head,
/// rendered on the audio thread, and a tail that a helper thread computes ahead of time.
///
/// Output block k is Σ_i X[k-i]·H[i] over the IR partitions H[i]. Only the first
/// `kHeadSegments` terms depend on recent input; the rest ("the tail") need input that is at
/// least `kHeadSegments` blocks old, so a worker can compute the tail for block k+kHeadSegments
/// as soon as block k is in. The audio thread never waits: a tail that is not ready in time is
/// computed inline for that block.
class Convolver {
  using aligned_vec_complex =
      std::vector<std::complex<float>, AlignedAllocator<std::complex<float>, 16>>;

 public:
  /// IR partitions rendered on audio thread.
  /// (8 × 128 frames ≈ 21 ms at 48 kHz).
  static constexpr size_t kHeadSegments = 8;

  Convolver();
  bool init(size_t blockSize, const AudioArray &ir, size_t irLen);
  void process(const DSPAudioArray &input, DSPAudioArray &output);

  /// @brief Whether the IR is long enough for part of it to be computed ahead of time.
  [[nodiscard]] bool hasTail() const {
    return _segCount > kHeadSegments;
  }

  /// @brief Tail block that became computable with the latest `process()` call; hand it to
  /// `computeTail` on a helper thread right after `process()` returns.
  /// @note Audio thread only.
  [[nodiscard]] size_t nextTailBlock() const {
    return _blocksProcessed.load(std::memory_order_relaxed) - 1 + kHeadSegments;
  }

  /// @brief Computes the tail for `block` and publishes it for the audio thread.
  /// @note Any thread. Safe to call after `process()` moved past `block`: a late result is
  /// published but never read, and a call older than the slot's current block does nothing.
  /// @note Reads the input delay line without a lock. The line keeps `kHeadSegments` spare
  /// slots, so the data stays valid unless the worker is more than 2 × kHeadSegments blocks
  /// late — a result that late is never read, since `process()` already moved past `block`.
  void computeTail(size_t block);

  void reset();
  [[nodiscard]] size_t getSegCount() const {
    return _trueSegmentCount;
  }

 private:
  /// `state` packs the block the spectrum belongs to with a busy bit, so claiming the slot and
  /// checking that no newer block owns it is one CAS.
  struct TailSlot {
    std::atomic<size_t> state{kEmptySlot};
    aligned_vec_complex spectrum;
  };
  static constexpr size_t kEmptySlot = 0;
  static constexpr size_t kBusyBit = 1;
  static constexpr size_t kTailSlotCount = kHeadSegments + 1;

  /// returns the number that indicates that this block is finished processing
  /// `10` maps to the first block, `100` maps to the second block, `110` maps to the third block, etc.
  static constexpr size_t readyState(size_t block) {
    return (block + 1) << 1;
  }

  [[nodiscard]] const aligned_vec_complex &inputSpectrum(size_t block, size_t age) const;
  void accumulateSegments(size_t block, size_t fromSegment, aligned_vec_complex &into) const;

  size_t _trueSegmentCount;
  size_t _blockSize;
  size_t _segSize;
  size_t _segCount;
  size_t _fftComplexSize;
  /// Delay line of input spectra, one per block, `_segCount + kHeadSegments` slots.
  std::vector<aligned_vec_complex> _segments;
  std::vector<aligned_vec_complex> _segmentsIR;
  std::unique_ptr<DSPAudioArray> _fftBuffer;
  std::shared_ptr<dsp::FFT> _fft;
  aligned_vec_complex _preMultiplied;
  std::unique_ptr<DSPAudioArray> _inputBuffer;
  std::vector<TailSlot> _tailSlots;

  /// Count of blocks whose spectrum is in the delay line; published by the audio thread.
  std::atomic<size_t> _blocksProcessed{0};
  size_t _tailMisses{0};

  /// Test-only window on the prefetch bookkeeping; hit and miss produce identical output.
  friend struct ConvolverInspector;
  friend void pairwise_complex_multiply_fast(
      const aligned_vec_complex &ir,
      const aligned_vec_complex &audio,
      aligned_vec_complex &pre);
};
} // namespace audioapi
