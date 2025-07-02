#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/dsp/Convolver.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <chrono>
#include <iostream>

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
  _segments.clear();
  _segmentsIR.clear();
  _fftBuffer.zero();
  _fft = nullptr;
  _preMultiplied.clear();
  _current = 0;
  _inputBuffer.zero();
}

bool Convolver::init(
    size_t blockSize,
    const audioapi::AudioArray &ir,
    size_t irLen) {
  reset();
  // blockSize must be a power of two
  if ((blockSize & (blockSize - 1))) {
    return false;
  }

  // Ignore zeros at the end of the impulse response because they only waste
  // computation time
  while (irLen > 0 && ::fabs(ir[irLen - 1]) < 0.000001f) {
    --irLen;
  }

  if (irLen == 0) {
    return true;
  }

  _blockSize = blockSize;
  // The length-N is split into P = N/B length-B sub filters
  _segCount = (size_t)(std::ceil((float)irLen / (float)_blockSize));
  _segSize = 2 * _blockSize;
  // size of the FFT is 2B, so the complex size is B+1, due to the
  // complex-conjugate symmetricity
  _fftComplexSize = _segSize / 2 + 1;
  _fft = std::make_shared<dsp::FFT>((int)_segSize);
  _fftBuffer.resize(_segSize);

  // segments preparation
  for (int i = 0; i < _segCount; ++i) {
    std::vector<std::complex<float>> vec(
        _fftComplexSize, std::complex<float>(0.0f, 0.0f));
    _segments.push_back(vec);
  }

  // ir preparation
  for (int i = 0; i < _segCount; ++i) {
    std::vector<std::complex<float>> segment(_fftComplexSize);
    // Each sub filter is zero-padded to length 2B and transformed using a
    // 2B-point real-to-complex FFT.
    memcpy(
        _fftBuffer.getData(),
        ir.getData() + i * _blockSize,
        _blockSize * sizeof(float));
    memset(_fftBuffer.getData() + _blockSize, 0, _blockSize * sizeof(float));
    _fft->doFFT(_fftBuffer.getData(), segment);
    _segmentsIR.push_back(segment);
  }

  _preMultiplied = std::vector<std::complex<float>>(_fftComplexSize);
  _inputBuffer.resize(_segSize);
  _current = 0;

  return true;
}

void Convolver::process(
    const audioapi::AudioArray &input,
    audioapi::AudioArray &output,
    size_t len) {
  if (_segCount == 0) {
    output.zero();
    return;
  }

  // The input buffer acts as a 2B-point sliding window of the input signal.
  // With each new input block, the right half of the input buffer is shifted
  // to the left and the new block is stored in the right half.
  memcpy(
      _inputBuffer.getData(),
      _inputBuffer.getData() + _blockSize,
      _blockSize * sizeof(float));
  memcpy(
      _inputBuffer.getData() + _blockSize,
      input.getData(),
      std::min(len, _blockSize) * sizeof(float));

  bool verbose = false;
  if (verbose) {
    for (int i = 0; i < _segSize; i++) {
      printf("inputBuffer[%d]: %f\n", i, _inputBuffer.getData()[i]);
    }
  }

  // All contents (DFT spectra) in the FDL are shifted up by one slot.
  // A 2B-point real-to-complex FFT is computed from the input buffer,
  // resulting in B+1 complex-conjugate symmetric DFT coefficients. The
  // result is stored in the first FDL slot.
  // _current marks first FDL slot, which is the current input block.
  _fft->doFFT(_inputBuffer.getData(), _segments[_current]);
  _fft->doInverseFFT(_segments[_current], _fftBuffer.getData());
  for (int i = 0; i < _segSize; ++i) {
    if (verbose) {
      printf(
          "fft[%d]: %f %f\n",
          i,
          _inputBuffer.getData()[i],
          _fftBuffer.getData()[i]);
    }
  }

  // The P sub filter spectra are pairwisely multiplied with the input spectra
  // in the FDL. The results are accumulated in the frequency-domain.
  std::fill(
      _preMultiplied.begin(),
      _preMultiplied.end(),
      std::complex<float>(0.0f, 0.0f));
  for (int i = 0; i < _segCount; ++i) {
    const int indexIr = i;
    const int indexAudio = (_current + i) % _segCount;
    for (int j = 0; j < _fftComplexSize; ++j) {
      _preMultiplied[j] += _segmentsIR[indexIr][j] * _segments[indexAudio][j];
    }
  }
  // move pointer to the next FDL slot
  _current = _current > 0 ? _current - 1 : _segCount - 1;

  // Of the accumulated spectral convolutions, an 2B-point complex-to-real
  // IFFT is computed. From the resulting 2B samples, the left half is
  // discarded and the right half is returned as the next output block.
  _fft->doInverseFFT(_preMultiplied, _fftBuffer.getData());
  if (verbose) {
    for (int i = 0; i < _fftComplexSize; i++) {
      printf(
          "premultiplied[%d]: %f + %fi\n",
          i,
          _preMultiplied[i].real(),
          _preMultiplied[i].imag());
    }
    for (int i = 0; i < _segSize; i++) {
      printf("fftBuffer[%d]: %f\n", i, _fftBuffer.getData()[i]);
    }
  }
  memcpy(
      output.getData(),
      _fftBuffer.getData() + _blockSize,
      std::min(len, _blockSize) * sizeof(float));
}
} // namespace audioapi
