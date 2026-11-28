#pragma once

#include <audioapi/dsp/FFT.h>
#include <audioapi/utils/AlignedAllocator.hpp>
#include <audioapi/utils/AudioArray.hpp>
#include <atomic>
#include <complex>
#include <cstring>
#include <memory>
#include <optional>
#include <vector>

namespace audioapi {

/// @brief Two-stage partitioned FFT convolver. The head of the impulse response runs in
/// block-size partitions on the audio thread; the tail runs in `TAIL_BLOCK_FRAMES` partitions
/// that a helper thread computes one tail block ahead of time.
class Convolver {
  using aligned_vec_complex =
      std::vector<std::complex<float>, AlignedAllocator<std::complex<float>, 16>>;

 public:
  /// Partition size of the tail; the tail is multiplied once per this many frames
  /// (2048 frames ≈ 43 ms at 48 kHz).
  static constexpr size_t TAIL_BLOCK_FRAMES = 2048;
  /// IR frames rendered on the audio thread. A tail block needs `TAIL_BLOCK_FRAMES` of input
  /// before it can be computed; the second block is the time the helper thread gets for it.
  static constexpr size_t HEAD_FRAMES = 2 * TAIL_BLOCK_FRAMES;

  Convolver();
  bool init(size_t blockSize, const AudioArray &ir, size_t irLen);
  /// @brief Convolves the first @p framesToProcess frames of @p input into @p output with no
  /// added latency. @p framesToProcess may differ on every call and need not relate to the block
  /// size: a call that ends mid-block convolves the partial block as if the rest were silent.
  void process(const DSPAudioArray &input, DSPAudioArray &output, size_t framesToProcess);

  /// @brief Whether the IR is long enough for part of it to be computed ahead of time.
  [[nodiscard]] bool hasTail() const {
    return _tailSegCount > 0;
  }

  /// @brief Tail block whose input the latest `process()` call completed, handed out once;
  /// pass it to `computeTail` on a helper thread right after `process()` returns. A call long
  /// enough to complete several tail blocks hands out the newest; it rendered the older ones.
  /// @note Audio thread only.
  [[nodiscard]] std::optional<size_t> takeCompletedTailBlock();

  /// @brief Computes tail block `block` and publishes it for the audio thread.
  /// @note Any thread. Does nothing for a block whose input is not complete, that `process()`
  /// already rendered, or whose slot holds the same or a newer block.
  /// @note Reads the tail delay line without a lock. The line keeps two spare slots, so what a
  /// worker reads stays valid until the input of `block + 3` completes. `process()` started
  /// rendering `block` well before that, and a result that late is never read.
  void computeTail(size_t block);

  void reset();
  [[nodiscard]] size_t getSegCount() const {
    return _trueSegmentCount;
  }

 private:
  /// `state` packs the tail block the frames belong to with a busy bit, so claiming the slot
  /// and checking that no newer block owns it is one CAS.
  struct TailSlot {
    std::atomic<size_t> state{EMPTY_SLOT};
    aligned_vec_complex spectrum;
    /// Inverse FFT of `spectrum`; the right half is the tail block.
    std::unique_ptr<DSPAudioArray> frames;
    /// An FFT keeps scratch memory, so the worker cannot share the audio thread's.
    std::unique_ptr<dsp::FFT> fft;
  };
  static constexpr size_t EMPTY_SLOT = 0;
  static constexpr size_t BUSY_BIT = 1;
  /// Two tail blocks are in flight at a time; the third slot lets a worker finish one block
  /// late without costing the next block its slot.
  static constexpr size_t TAIL_SLOT_COUNT = 3;
  /// Tail blocks between the input block that completes a tail block and its first frame
  /// in the output.
  static constexpr size_t TAIL_DELAY_BLOCKS = HEAD_FRAMES / TAIL_BLOCK_FRAMES;
  /// Tail delay line slots beyond the tail's partitions: the block the audio thread writes
  /// while a worker reads, and one more for a worker that is late.
  static constexpr size_t SPARE_TAIL_SEGMENTS = 2;

  /// @brief Slot state meaning "tail block `block` is complete". Always even (bit 0 is
  /// `BUSY_BIT`) and above `EMPTY_SLOT`; ordered like the blocks, so `>=` compares block
  /// numbers. Blocks 0, 1, 2 map to 0b10, 0b100, 0b110.
  static constexpr size_t readyState(size_t block) {
    return (block + 1) << 1U;
  }

  void initTail(const AudioArray &ir, size_t irLen);
  [[nodiscard]] const aligned_vec_complex &inputSpectrum(size_t block, size_t age) const;
  [[nodiscard]] const aligned_vec_complex &tailInputSpectrum(size_t block, size_t age) const;
  void accumulateCompletedBlocks();
  void advanceBlock();
  void accumulateTail(size_t block, aligned_vec_complex &into) const;
  /// @brief Adds the tail to @p frames frames of @p output starting at @p outputStart and
  /// buffers the same input frames, found at @p windowStart of the head's input window, for the
  /// tail block being collected.
  void renderTail(DSPAudioArray &output, size_t outputStart, size_t windowStart, size_t frames);
  void loadTailBlock(size_t block);
  void completeTailInputBlock();

  size_t _trueSegmentCount;
  size_t _blockSize;
  size_t _segSize;
  size_t _headSegCount;
  size_t _fftComplexSize;
  /// Delay line of head input spectra, one per block, `_headSegCount` slots.
  std::vector<aligned_vec_complex> _segments;
  std::vector<aligned_vec_complex> _segmentsIR;
  std::unique_ptr<DSPAudioArray> _fftBuffer;
  std::shared_ptr<dsp::FFT> _fft;
  /// Contribution of every completed input block, fixed for the whole block being filled.
  aligned_vec_complex _preMultiplied;
  /// `_preMultiplied` plus the partial block's own contribution.
  aligned_vec_complex _blockSpectrum;
  std::unique_ptr<DSPAudioArray> _inputBuffer;
  /// Count of completed input blocks, which is also the index of the block being filled.
  size_t _blocksProcessed{0};
  /// Frames of the block being filled already received.
  size_t _inputFill{0};

  size_t _tailSegCount;
  /// Delay line of tail input spectra, one per tail block, `_tailSegCount + SPARE_TAIL_SEGMENTS`
  /// slots.
  /// Only the audio thread writes it.
  std::vector<aligned_vec_complex> _tailSegments;
  std::vector<aligned_vec_complex> _tailSegmentsIR;
  std::unique_ptr<dsp::FFT> _tailFft;
  /// Sliding window of the last `2 × TAIL_BLOCK_FRAMES` input frames.
  std::unique_ptr<DSPAudioArray> _tailInputBuffer;
  aligned_vec_complex _tailPreMultiplied;
  /// The tail block being rendered sits in the right half.
  std::unique_ptr<DSPAudioArray> _tailOutput;
  std::vector<TailSlot> _tailSlots;
  /// Frames of the current tail block already rendered, and of the next one already buffered.
  size_t _tailPosition{0};
  bool _completedTailBlockPending{false};

  /// Count of tail blocks whose spectrum is in the delay line; published by the audio thread.
  std::atomic<size_t> _tailBlocksCompleted{0};
  /// Count of tail blocks the audio thread started rendering; a later result is unused.
  std::atomic<size_t> _tailBlocksRendered{0};
  size_t _tailMisses{0};

  /// Test-only window on the prefetch bookkeeping; hit and miss produce identical output.
  friend struct ConvolverInspector;
  friend void pairwise_complex_multiply_fast(
      const aligned_vec_complex &ir,
      const aligned_vec_complex &audio,
      aligned_vec_complex &pre);
};
} // namespace audioapi
