#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/dsp/Windows.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/CircularAudioArray.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {
AnalyserNode::AnalyserNode(std::shared_ptr<BaseAudioContext> context)
    : AudioNode(context),
      fftSize_(2048),
      minDecibels_(-100),
      maxDecibels_(-30),
      smoothingTimeConstant_(0.8),
      windowType_(WindowType::BLACKMAN),
      inputBuffer_(std::make_unique<CircularAudioArray>(MAX_FFT_SIZE * 2)),
      downMixBus_(std::make_unique<AudioBus>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())),
      tempBuffer_(std::make_unique<AudioArray>(fftSize_)),
      fft_(std::make_unique<dsp::FFT>(fftSize_)),
      complexData_(std::vector<std::complex<float>>(fftSize_)),
      magnitudeBuffer_(std::make_unique<AudioArray>(fftSize_ / 2)) {
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

std::string AnalyserNode::getWindowType() const {
  return AnalyserNode::toString(windowType_);
}

void AnalyserNode::setFftSize(int fftSize) {
  if (fftSize_ == fftSize) {
    return;
  }

  fftSize_ = fftSize;
  fft_ = std::make_unique<dsp::FFT>(fftSize_);
  complexData_ = std::vector<std::complex<float>>(fftSize_);
  magnitudeBuffer_ = std::make_unique<AudioArray>(fftSize_ / 2);
  tempBuffer_ = std::make_unique<AudioArray>(fftSize_);
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

void AnalyserNode::setWindowType(const std::string &type) {
  setWindowData(windowType_, fftSize_);
  windowType_ = AnalyserNode::fromString(type);
}

void AnalyserNode::getFloatFrequencyData(float *data, int length) {
  doFFTAnalysis();

  length = std::min(static_cast<int>(magnitudeBuffer_->getSize()), length);
  dsp::linearToDecibels(magnitudeBuffer_->getData(), data, length);
}

void AnalyserNode::getByteFrequencyData(uint8_t *data, int length) {
  doFFTAnalysis();

  auto magnitudeBufferData = magnitudeBuffer_->getData();
  length = std::min(static_cast<int>(magnitudeBuffer_->getSize()), length);

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
  inputBuffer_->pop_back(data, size, std::max(0, fftSize_ - size), true);
}

void AnalyserNode::getByteTimeDomainData(uint8_t *data, int length) {
  auto size = std::min(fftSize_, length);

  inputBuffer_->pop_back(tempBuffer_->getData(), fftSize_, std::max(0, fftSize_ - size), true);

  for (int i = 0; i < size; i++) {
    auto value = tempBuffer_->getData()[i];

    float scaledValue = 128 * (value + 1);

    if (scaledValue < 0) {
      scaledValue = 0;
    }
    if (scaledValue > UINT8_MAX) {
      scaledValue = UINT8_MAX;
    }

    data[i] = static_cast<uint8_t>(scaledValue);
  }
}

std::shared_ptr<AudioBus> AnalyserNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  // Analyser should behave like a sniffer node, it should not modify the
  // processingBus but instead copy the data to its own input buffer.

  // Down mix the input bus to mono
  downMixBus_->copy(processingBus.get());
  // Copy the down mixed bus to the input buffer (circular buffer)
  inputBuffer_->push_back(downMixBus_->getChannel(0)->getData(), framesToProcess, true);

  shouldDoFFTAnalysis_ = true;

  return processingBus;
}

void AnalyserNode::doFFTAnalysis() {
  if (!shouldDoFFTAnalysis_) {
    return;
  }

  shouldDoFFTAnalysis_ = false;

  // We want to copy last fftSize_ elements added to the input buffer to apply
  // the window.
  inputBuffer_->pop_back(tempBuffer_->getData(), fftSize_, 0, true);

  dsp::multiply(tempBuffer_->getData(), windowData_->getData(), tempBuffer_->getData(), fftSize_);

  // do fft analysis - get frequency domain data
  fft_->doFFT(tempBuffer_->getData(), complexData_);

  // Zero out nquist component
  complexData_[0] = std::complex<float>(complexData_[0].real(), 0);

  const float magnitudeScale = 1.0f / static_cast<float>(fftSize_);
  auto magnitudeBufferData = magnitudeBuffer_->getData();

  for (int i = 0; i < magnitudeBuffer_->getSize(); i++) {
    auto scalarMagnitude = std::abs(complexData_[i]) * magnitudeScale;
    magnitudeBufferData[i] = static_cast<float>(
        smoothingTimeConstant_ * magnitudeBufferData[i] +
        (1 - smoothingTimeConstant_) * scalarMagnitude);
  }
}

void AnalyserNode::setWindowData(audioapi::AnalyserNode::WindowType type, int size) {
  if (windowType_ == type && windowData_ && windowData_->getSize() == size) {
    return;
  }

  if (!windowData_ || windowData_->getSize() != size) {
    windowData_ = std::make_shared<AudioArray>(size);
  }

  switch (windowType_) {
    case WindowType::BLACKMAN:
      dsp::Blackman().apply(windowData_->getData(), static_cast<int>(windowData_->getSize()));
      break;
    case WindowType::HANN:
      dsp::Hann().apply(windowData_->getData(), static_cast<int>(windowData_->getSize()));
      break;
  }
}
} // namespace audioapi
