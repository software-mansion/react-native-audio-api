#include "AnalyserNode.h"

#include <complex>

#include "AudioArray.h"
#include "AudioBus.h"
#include "AudioUtils.h"
#include "BaseAudioContext.h"
#include "FFTFrame.h"
#include "VectorMath.h"

namespace audioapi {
AnalyserNode::AnalyserNode(audioapi::BaseAudioContext *context)
    : AudioNode(context),
      fftSize_(DEFAULT_FFT_SIZE),
      minDecibels_(DEFAULT_MIN_DECIBELS),
      maxDecibels_(DEFAULT_MAX_DECIBELS),
      smoothingTimeConstant_(DEFAULT_SMOOTHING_TIME_CONSTANT),
      windowType_(WindowType::BLACKMAN),
      vWriteIndex_(0) {
  inputBuffer_ = std::make_unique<AudioArray>(MAX_FFT_SIZE * 2);
  magnitudeBuffer_ = std::make_unique<AudioArray>(fftSize_ / 2);
  downMixBus_ = std::make_unique<AudioBus>(
      context_->getSampleRate(), RENDER_QUANTUM_SIZE, 1);

  fftFrame_ = std::make_unique<FFTFrame>(fftSize_);

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
  fftFrame_ = std::make_unique<FFTFrame>(fftSize_);
  magnitudeBuffer_ = std::make_unique<AudioArray>(fftSize_ / 2);
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
  windowType_ = AnalyserNode::fromString(type);
}

void AnalyserNode::getFloatFrequencyData(float *data, int length) {
  doFFTAnalysis();

  length = std::min(static_cast<int>(magnitudeBuffer_->getSize()), length);
  VectorMath::linearToDecibels(magnitudeBuffer_->getData(), data, length);
}

void AnalyserNode::getByteFrequencyData(uint8_t *data, int length) {
  doFFTAnalysis();

  auto magnitudeBufferData = magnitudeBuffer_->getData();
  length = std::min(static_cast<int>(magnitudeBuffer_->getSize()), length);

  const auto rangeScaleFactor =
      maxDecibels_ == minDecibels_ ? 1 : 1 / (maxDecibels_ - minDecibels_);

  for (int i = 0; i < length; i++) {
    auto dbMag = magnitudeBufferData[i] == 0
        ? minDecibels_
        : AudioUtils::linearToDecibels(magnitudeBufferData[i]);
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

  for (int i = 0; i < size; i++) {
    data[i] = inputBuffer_->getData()
                  [(vWriteIndex_ + i - fftSize_ + inputBuffer_->getSize()) %
                   inputBuffer_->getSize()];
  }
}

void AnalyserNode::getByteTimeDomainData(uint8_t *data, int length) {
  auto size = std::min(fftSize_, length);

  for (int i = 0; i < size; i++) {
    auto value = inputBuffer_->getData()
                     [(vWriteIndex_ + i - fftSize_ + inputBuffer_->getSize()) %
                      inputBuffer_->getSize()];

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

void AnalyserNode::processNode(
    audioapi::AudioBus *processingBus,
    int framesToProcess) {
  // Analyser should behave like a sniffer node, it should not modify the
  // processingBus but instead copy the data to its own input buffer.

  // MIXING
  downMixBus_->copy(processingBus);

  if (vWriteIndex_ + framesToProcess > inputBuffer_->getSize()) {
    auto framesToCopy =
        static_cast<int>(inputBuffer_->getSize()) - vWriteIndex_;
    memcpy(
        inputBuffer_->getData() + vWriteIndex_,
        downMixBus_->getChannel(0)->getData(),
        framesToCopy * sizeof(float));

    vWriteIndex_ = 0;
    framesToProcess -= framesToCopy;
  }

  memcpy(
      inputBuffer_->getData() + vWriteIndex_,
      downMixBus_->getChannel(0)->getData(),
      framesToProcess * sizeof(float));

  vWriteIndex_ += framesToProcess;
  if (vWriteIndex_ >= inputBuffer_->getSize()) {
    vWriteIndex_ = 0;
  }

  shouldDoFFTAnalysis_ = true;
}

void AnalyserNode::doFFTAnalysis() {
  if (!shouldDoFFTAnalysis_) {
    return;
  }

  shouldDoFFTAnalysis_ = false;

  // We need to copy the fftSize elements from input buffer to a temporary
  // buffer to apply the window.
  AudioArray tempBuffer(this->fftSize_);

  // We want to copy last fftSize_ elements added to the input buffer(fftSize_
  // elements before vWriteIndex_). However inputBuffer_ works like a circular
  // buffer so we have two cases to consider.
  if (vWriteIndex_ < fftSize_) {
    tempBuffer.copy(
        inputBuffer_.get(),
        vWriteIndex_ - fftSize_ + inputBuffer_->getSize(),
        0,
        fftSize_ - vWriteIndex_);
    tempBuffer.copy(
        inputBuffer_.get(), 0, fftSize_ - vWriteIndex_, vWriteIndex_);
  } else {
    tempBuffer.copy(inputBuffer_.get(), vWriteIndex_ - fftSize_, 0, fftSize_);
  }

  switch (windowType_) {
    case WindowType::BLACKMAN:
      AnalyserNode::applyBlackManWindow(tempBuffer.getData(), fftSize_);
      break;
    case WindowType::HANN:
      AnalyserNode::applyHannWindow(tempBuffer.getData(), fftSize_);
      break;
  }

  // do fft analysis - get frequency domain data
  fftFrame_->doFFT(tempBuffer.getData());

  auto *realFFTFrameData = fftFrame_->getRealData();
  auto *imaginaryFFTFrameData = fftFrame_->getImaginaryData();

  // Zero out nquist component
  imaginaryFFTFrameData[0] = 0.0f;

  const float magnitudeScale = 1.0f / static_cast<float>(fftSize_);
  auto magnitudeBufferData = magnitudeBuffer_->getData();

  for (int i = 0; i < magnitudeBuffer_->getSize(); i++) {
    std::complex<float> c(realFFTFrameData[i], imaginaryFFTFrameData[i]);
    auto scalarMagnitude = std::abs(c) * magnitudeScale;
    magnitudeBufferData[i] = static_cast<float>(
        smoothingTimeConstant_ * magnitudeBufferData[i] +
        (1 - smoothingTimeConstant_) * scalarMagnitude);
  }
}

void AnalyserNode::applyBlackManWindow(float *data, int length) {
  // https://www.sciencedirect.com/topics/engineering/blackman-window
  // https://docs.scipy.org/doc//scipy-1.2.3/reference/generated/scipy.signal.windows.blackman.html#scipy.signal.windows.blackman

  for (int i = 0; i < length; ++i) {
    auto x = static_cast<float>(i) / static_cast<float>(length);
    auto window = 0.42f - 0.5f * cos(2 * PI * x) + 0.08f * cos(4 * PI * x);
    data[i] *= window;
  }
}

void AnalyserNode::applyHannWindow(float *data, int length) {
  // https://www.sciencedirect.com/topics/engineering/hanning-window
  // https://docs.scipy.org/doc//scipy-1.2.3/reference/generated/scipy.signal.windows.hann.html#scipy.signal.windows.hann
  for (int i = 0; i < length; ++i) {
    auto x = static_cast<float>(i) / static_cast<float>(length - 1);
    auto window = 0.5f - 0.5f * cos(2 * PI * x);
    data[i] *= window;
  }
}
} // namespace audioapi
