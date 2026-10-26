#pragma once

#include <audioapi/dsp/FFT.h>
#include <audioapi/utils/AlignedAllocator.hpp>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <complex>
#include <cstring>
#include <memory>
#include <vector>

namespace audioapi {

class Convolver {
  using aligned_vec_complex =
      std::vector<std::complex<float>, AlignedAllocator<std::complex<float>, 16>>;

 public:
  Convolver();
  bool init(size_t blockSize, const AudioArray &ir, size_t irLen);
  /// @brief Convolves the first @p framesToProcess frames of @p input into @p output with no
  /// added latency. @p framesToProcess may differ on every call and need not relate to the block
  /// size: a call that ends mid-block convolves the partial block as if the rest were silent.
  void process(const DSPAudioArray &input, DSPAudioArray &output, size_t framesToProcess);
  void reset();
  [[nodiscard]] size_t getSegCount() const {
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
  std::unique_ptr<DSPAudioArray> _fftBuffer;
  std::shared_ptr<dsp::FFT> _fft;
  /// Contribution of every completed input block, fixed for the whole block being filled.
  aligned_vec_complex _preMultiplied;
  /// `_preMultiplied` plus the partial block's own contribution.
  aligned_vec_complex _blockSpectrum;
  size_t _current;
  /// Frames of the current block already received.
  size_t _inputFill;
  std::unique_ptr<DSPAudioArray> _inputBuffer;

  void accumulateCompletedBlocks();
  void advanceBlock();

  friend void pairwise_complex_multiply_fast(
      const aligned_vec_complex &ir,
      const aligned_vec_complex &audio,
      aligned_vec_complex &pre);
};
} // namespace audioapi
