#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/dsp/Windows.hpp>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
#include <audioapi/utils/CircularAudioArray.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

AnalyserNode::AnalyserNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const AnalyserOptions &options)
    : AudioNode(context, options),
      fftSize_(options.fftSize),
      minDecibels_(options.minDecibels),
      maxDecibels_(options.maxDecibels),
      smoothingTimeConstant_(options.smoothingTimeConstant),
      windowType_(WindowType::BLACKMAN),
      inputArray_(std::make_unique<CircularAudioArray>(MAX_FFT_SIZE * 2)),
      downMixBuffer_(
          std::make_unique<AudioBuffer>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())),
      tempArray_(std::make_unique<AudioArray>(fftSize_)),
      fft_(std::make_unique<dsp::FFT>(fftSize_)),
      complexData_(std::vector<std::complex<float>>(fftSize_)),
      magnitudeArray_(std::make_unique<AudioArray>(fftSize_ / 2)) {
  setWindowData(windowType_, fftSize_);
  isInitialized_ = true;
}

int AnalyserNode::getFftSize() const {
  return fftSize_;
}

int AnalyserNode::getFrequencyBinCount() const {
  return fftSize_ / 2;
}

float AnalyserNode::getMinDecibels() const {
  return minDecibels_;
}

float AnalyserNode::getMaxDecibels() const {
  return maxDecibels_;
}

float AnalyserNode::getSmoothingTimeConstant() const {
  return smoothingTimeConstant_;
}

AnalyserNode::WindowType AnalyserNode::getWindowType() const {
  return windowType_;
}

void AnalyserNode::setFftSize(int fftSize) {
  if (fftSize_ == fftSize) {
    return;
  }

  fftSize_ = fftSize;
  fft_ = std::make_unique<dsp::FFT>(fftSize_);
  complexData_ = std::vector<std::complex<float>>(fftSize_);
  magnitudeArray_ = std::make_unique<AudioArray>(fftSize_ / 2);
  tempArray_ = std::make_unique<AudioArray>(fftSize_);
  setWindowData(windowType_, fftSize_);
}

void AnalyserNode::setMinDecibels(float minDecibels) {
  minDecibels_ = minDecibels;
}

void AnalyserNode::setMaxDecibels(float maxDecibels) {
  maxDecibels_ = maxDecibels;
}

void AnalyserNode::setSmoothingTimeConstant(float smoothingTimeConstant) {
  smoothingTimeConstant_ = smoothingTimeConstant;
}

void AnalyserNode::setWindowType(AnalyserNode::WindowType type) {
  setWindowData(type, fftSize_);
}

void AnalyserNode::getFloatFrequencyData(float *data, int length) {
  doFFTAnalysis();

  length = std::min(static_cast<int>(magnitudeArray_->getSize()), length);
  auto magnitudeSpan = magnitudeArray_->span();

  for (int i = 0; i < length; i++) {
    data[i] = dsp::linearToDecibels(magnitudeSpan[i]);
  }
}

void AnalyserNode::getByteFrequencyData(uint8_t *data, int length) {
  doFFTAnalysis();

  auto magnitudeBufferData = magnitudeArray_->span();
  length = std::min(static_cast<int>(magnitudeArray_->getSize()), length);

  const auto rangeScaleFactor =
      maxDecibels_ == minDecibels_ ? 1 : 1 / (maxDecibels_ - minDecibels_);

  for (int i = 0; i < length; i++) {
    auto dbMag =
        magnitudeBufferData[i] == 0 ? minDecibels_ : dsp::linearToDecibels(magnitudeBufferData[i]);
    auto scaledValue = UINT8_MAX * (dbMag - minDecibels_) * rangeScaleFactor;

    if (scaledValue < 0) {
      scaledValue = 0;
    }
    if (scaledValue > UINT8_MAX) {
      scaledValue = UINT8_MAX;
    }

    data[i] = static_cast<uint8_t>(scaledValue);
  }
}

void AnalyserNode::getFloatTimeDomainData(float *data, int length) {
  auto size = std::min(fftSize_, length);

  inputArray_->pop_back(data, size, std::max(0, fftSize_ - size), true);
}

void AnalyserNode::getByteTimeDomainData(uint8_t *data, int length) {
  auto size = std::min(fftSize_, length);

  inputArray_->pop_back(*tempArray_, size, std::max(0, fftSize_ - size), true);

  auto values = tempArray_->span();

  for (int i = 0; i < size; i++) {
    float scaledValue = 128 * (values[i] + 1);

    if (scaledValue < 0) {
      scaledValue = 0;
    }
    if (scaledValue > UINT8_MAX) {
      scaledValue = UINT8_MAX;
    }

    data[i] = static_cast<uint8_t>(scaledValue);
  }
}

std::shared_ptr<AudioBuffer> AnalyserNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  // Analyser should behave like a sniffer node, it should not modify the
  // processingBuffer but instead copy the data to its own input buffer.

  // Down mix the input buffer to mono
  downMixBuffer_->copy(*processingBuffer);
  // Copy the down mixed buffer to the input buffer (circular buffer)
  inputArray_->push_back(*downMixBuffer_->getChannel(0), framesToProcess, true);

  shouldDoFFTAnalysis_ = true;

  return processingBuffer;
}

void AnalyserNode::doFFTAnalysis() {
  if (!shouldDoFFTAnalysis_) {
    return;
  }

  shouldDoFFTAnalysis_ = false;

  // We want to copy last fftSize_ elements added to the input buffer to apply
  // the window.
  inputArray_->pop_back(*tempArray_, fftSize_, 0, true);

  tempArray_->multiply(*windowData_, fftSize_);

  // do fft analysis - get frequency domain data
  fft_->doFFT(*tempArray_, complexData_);

  // Zero out nquist component
  complexData_[0] = std::complex<float>(complexData_[0].real(), 0);

  const float magnitudeScale = 1.0f / static_cast<float>(fftSize_);
  auto magnitudeBufferData = magnitudeArray_->span();

  for (int i = 0; i < magnitudeArray_->getSize(); i++) {
    auto scalarMagnitude = std::abs(complexData_[i]) * magnitudeScale;
    magnitudeBufferData[i] = static_cast<float>(
        smoothingTimeConstant_ * magnitudeBufferData[i] +
        (1 - smoothingTimeConstant_) * scalarMagnitude);
  }
}

void AnalyserNode::setWindowData(AnalyserNode::WindowType type, int size) {
  if (windowType_ == type && windowData_ != nullptr && windowData_->getSize() == size) {
    return;
  }

  windowType_ = type;
  if (windowData_ == nullptr || windowData_->getSize() != size) {
    windowData_ = std::make_shared<AudioArray>(size);
  }

  switch (windowType_) {
    case WindowType::BLACKMAN:
      dsp::Blackman().apply(windowData_->span());
      break;
    case WindowType::HANN:
      dsp::Hann().apply(windowData_->span());
      break;
  }
}
} // namespace audioapi
