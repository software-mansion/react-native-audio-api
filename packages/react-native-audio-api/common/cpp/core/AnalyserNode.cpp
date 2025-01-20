#include "AnalyserNode.h"

#include <complex>

#include "AudioArray.h"
#include "AudioBus.h"
#include "BaseAudioContext.h"
#include "FFTFrame.h"
#include "AudioUtils.h"
#include "VectorMath.h"


namespace audioapi {
AnalyserNode::AnalyserNode(audioapi::BaseAudioContext *context)
    : AudioNode(context),
      fftSize_(DEFAULT_FFT_SIZE),
      minDecibels_(DEFAULT_MIN_DECIBELS),
      maxDecibels_(DEFAULT_MAX_DECIBELS),
      smoothingTimeConstant_(DEFAULT_SMOOTHING_TIME_CONSTANT),
      vWriteIndex_(0) {
  inputBuffer_ = std::make_unique<AudioArray>(MAX_FFT_SIZE * 2);
  fftFrame_ = std::make_unique<FFTFrame>(fftSize_);
  magnitudeBuffer_ = std::make_unique<AudioArray>(DEFAULT_FFT_SIZE / 2);
  isInitialized_ = true;
}

size_t AnalyserNode::getFftSize() const {
  return fftSize_;
}

size_t AnalyserNode::getFrequencyBinCount() const {
  return fftSize_ / 2;
}

double AnalyserNode::getMinDecibels() const {
  return minDecibels_;
}

double AnalyserNode::getMaxDecibels() const {
  return maxDecibels_;
}

double AnalyserNode::getSmoothingTimeConstant() const {
  return smoothingTimeConstant_;
}

void AnalyserNode::setFftSize(size_t fftSize) {
  if (fftSize_ == fftSize) {
    return;
  }

  fftSize_ = fftSize;
  fftFrame_ = std::make_unique<FFTFrame>(fftSize_);
}

void AnalyserNode::setMinDecibels(double minDecibels) {
  minDecibels_ = minDecibels;
}

void AnalyserNode::setMaxDecibels(double maxDecibels) {
  maxDecibels_ = maxDecibels;
}

void AnalyserNode::setSmoothingTimeConstant(double smoothingTimeConstant) {
  smoothingTimeConstant_ = smoothingTimeConstant;
}

void AnalyserNode::getFloatFrequencyData(float *data, size_t length) {
    doFFTAnalysis();

    length = std::min<size_t>(magnitudeBuffer_->getSize(), length);
    VectorMath::linearToDecibels(magnitudeBuffer_->getData(), data, length);
}

void AnalyserNode::getByteFrequencyData(uint8_t *data, size_t length) {
    doFFTAnalysis();

    auto magnitudeBufferData = magnitudeBuffer_->getData();
    length = std::min<size_t>(magnitudeBuffer_->getSize(), length);

    const auto rangeScaleFactor = maxDecibels_ == minDecibels_ ? 1 : 1 / (maxDecibels_ - minDecibels_);

    for (size_t i = 0; i < length; i++) {
        auto dbMag = magnitudeBufferData[i] == 0 ? minDecibels_ : AudioUtils::linearToDecibels(magnitudeBufferData[i]);
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

void AnalyserNode::getFloatTimeDomainData(float *data, size_t length) {
  auto size = std::min(fftSize_, length);

  for (size_t i = 0; i < size; i++) {
    data[i] = inputBuffer_->getData()
                  [(vWriteIndex_ + i - fftSize_ + inputBuffer_->getSize()) %
                   inputBuffer_->getSize()];
  }
}

void AnalyserNode::getByteTimeDomainData(uint8_t *data, size_t length) {
  auto size = std::min(fftSize_, length);

  for (size_t i = 0; i < size; i++) {
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
  if (!isInitialized_) {
    processingBus->zero();
    return;
  }

  if (downMixBus_ == nullptr) {
    downMixBus_ = std::make_unique<AudioBus>(
        context_->getSampleRate(), processingBus->getSize(), 1);
  }

  downMixBus_->copy(processingBus);

  if (vWriteIndex_ + framesToProcess > inputBuffer_->getSize()) {
    auto framesToCopy = inputBuffer_->getSize() - vWriteIndex_;
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

  AudioArray tempBuffer(this->fftSize_);

  if (vWriteIndex_ < fftSize_) {
      tempBuffer.copy(inputBuffer_.get(), vWriteIndex_ - fftSize_ + inputBuffer_->getSize(), fftSize_ - vWriteIndex_);
      tempBuffer.copy(inputBuffer_.get(), 0, fftSize_ - vWriteIndex_,  vWriteIndex_);

  } else {
      tempBuffer.copy(inputBuffer_.get(), vWriteIndex_ - fftSize_, fftSize_);
  }

    AnalyserNode::applyWindow(tempBuffer.getData(), fftSize_);

  fftFrame_->doFFT(tempBuffer.getData());

    auto *realFFTFrameData = fftFrame_->getRealData();
    auto *imaginaryFFTFrameData = fftFrame_->getImaginaryData();

    imaginaryFFTFrameData[0] = 0.0f;

    const float magnitudeScale = 1.0f / static_cast<float>(fftSize_);

    for (size_t i = 0; i< magnitudeBuffer_->getSize(); i++) {
        std::complex<float> c(realFFTFrameData[i], imaginaryFFTFrameData[i]);
        auto scalarMagnitude = std::abs(c) * magnitudeScale;
        magnitudeBuffer_->getData()[i] = static_cast<float>(smoothingTimeConstant_ * magnitudeBuffer_->getData()[i] + (1 - smoothingTimeConstant_) * scalarMagnitude);
    }
}

void AnalyserNode::applyWindow(float *data, size_t length) {
    // https://www.sciencedirect.com/topics/engineering/blackman-window
    auto alpha = 0.16f;
    auto a0 = 0.5f * (1 - alpha);
    auto a1 = 0.5f;
    auto a2 = 0.5f * alpha;

    for (int i = 0; i < length; ++i) {
        auto x = static_cast<float>(i) / static_cast<float>(length);
        auto window = a0 - a1 * cos(2 * static_cast<float>(M_PI) * x) + a2 * cos(4 * static_cast<float>(M_PI) * x);
        data[i] *= window;
    }
}
} // namespace audioapi
