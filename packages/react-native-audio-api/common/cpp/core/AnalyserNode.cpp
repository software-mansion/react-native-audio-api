#include "AnalyserNode.h"
#include "AudioArray.h"
#include "AudioBus.h"
#include "BaseAudioContext.h"
#include "FFTFrame.h"

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
  int log2size = static_cast<int>(log2(fftSize));
  bool isPowerOfTwo(1UL << log2size == fftSize);

  if (!isPowerOfTwo || fftSize < MIN_FFT_SIZE || fftSize > MAX_FFT_SIZE) {
    return;
  }

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

void AnalyserNode::getFloatFrequencyData(float *data, size_t length) {}

void AnalyserNode::getByteFrequencyData(float *data, size_t length) {}

void AnalyserNode::getFloatTimeDomainData(float *data, size_t length) {
  auto size = std::min(fftSize_, length);

  for (size_t i = 0; i < size; i++) {
    data[i] = inputBuffer_->getData()
                  [(vWriteIndex_ + i - fftSize_ + inputBuffer_->getSize()) %
                   inputBuffer_->getSize()];
  }
}

void AnalyserNode::getByteTimeDomainData(float *data, size_t length) {
  auto size = std::min(fftSize_, length);

  for (size_t i = 0; i < size; i++) {
    auto value = inputBuffer_->getData()
                     [(vWriteIndex_ + i - fftSize_ + inputBuffer_->getSize()) %
                      inputBuffer_->getSize()];

    float scaledValue = 128 * (value + 1);

    if (scaledValue < 0)
      scaledValue = 0;
    if (scaledValue > UCHAR_MAX)
      scaledValue = UCHAR_MAX;

    data[i] = scaledValue;
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
}
} // namespace audioapi
