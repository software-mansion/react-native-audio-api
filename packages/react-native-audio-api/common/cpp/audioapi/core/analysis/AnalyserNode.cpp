#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/types/NodeOptions.h>

#include <algorithm>
#include <memory>
#include <vector>

namespace audioapi {

AnalyserNode::AnalyserNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const AnalyserOptions &options)
    : AudioNode(context, options),
      inputArray_(std::make_unique<CircularDSPAudioArray>(MAX_FFT_SIZE * 2)),
      downMixBuffer_(
          std::make_unique<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())),
      minDecibels_(options.minDecibels),
      maxDecibels_(options.maxDecibels),
      smoothingTimeConstant_(options.smoothingTimeConstant) {
  setFFTSize(options.fftSize);
  setProcessableState(GraphObject::PROCESSABLE_STATE::ALWAYS_PROCESSABLE);
}

void AnalyserNode::setFFTSize(int fftSize) {
  if (fftSize == fftSize_.load(std::memory_order_acquire)) {
    return;
  }

  fft_ = std::make_unique<dsp::FFT>(fftSize);
  complexData_ = std::vector<std::complex<float>>(fftSize);
  magnitudeArray_ = std::make_unique<DSPAudioArray>(fftSize / 2);
  tempArray_ = std::make_unique<DSPAudioArray>(fftSize);
  initializeWindowData(fftSize);
  fftSize_.store(fftSize, std::memory_order_release);
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

    data[i] = static_cast<uint8_t>(std::clamp(scaledValue, 0.0f, static_cast<float>(UINT8_MAX)));
  }
}

void AnalyserNode::getFloatTimeDomainData(float *data, int length) {
  auto *frame = analysisBuffer_.getForReader();
  auto size = std::min(frame->fftSize, length);

  frame->timeDomain.copyTo(data, 0, 0, size);
}

void AnalyserNode::getByteTimeDomainData(uint8_t *data, int length) {
  auto *frame = analysisBuffer_.getForReader();
  auto size = std::min(frame->fftSize, length);

  auto values = frame->timeDomain.span();

  constexpr float BYTE_CENTER = 128.0f;
  for (int i = 0; i < size; i++) {
    float scaledValue = BYTE_CENTER * (values[i] + 1);
    scaledValue = std::clamp(scaledValue, 0.0f, static_cast<float>(UINT8_MAX));

    data[i] = static_cast<uint8_t>(scaledValue);
  }
}

void AnalyserNode::processNode(int framesToProcess) {
  // Analyser should behave like a sniffer node, it should not modify the
  // audioBuffer_ but instead copy the data to its own input buffer.

  // Down mix the input buffer to mono
  downMixBuffer_->copy(*audioBuffer_);
  // Copy the down mixed buffer to the input buffer (circular buffer)
  inputArray_->push_back(*downMixBuffer_->getChannel(0), framesToProcess, true);

  // Snapshot the latest fftSize_ samples into the triple buffer for the JS thread.
  auto *frame = analysisBuffer_.getForWriter();
  auto fftSize = fftSize_.load(std::memory_order_acquire);
  frame->fftSize = fftSize;
  frame->sequenceNumber = ++publishSequence_;
  inputArray_->pop_back(frame->timeDomain, fftSize, 0, true);
  analysisBuffer_.publish();
}

void AnalyserNode::doFFTAnalysis() {
  auto *frame = analysisBuffer_.getForReader();

  if (frame->sequenceNumber == lastAnalyzedSequence_) {
    return;
  }

  auto fftSize = frame->fftSize;

  // relaxed because fftSize_ is only updated on the JS thread.
  if (fftSize != fftSize_.load(std::memory_order_relaxed)) {
    return;
  }

  lastAnalyzedSequence_ = frame->sequenceNumber;

  // Copy the snapshot from the triple buffer and apply the window.
  tempArray_->copy(frame->timeDomain, 0, 0, fftSize);
  tempArray_->multiply(*windowData_, fftSize);

  // do fft analysis - get frequency domain data
  fft_->doFFT(*tempArray_, complexData_);

  // Zero out nquist component
  complexData_[0] = std::complex<float>(complexData_[0].real(), 0);

  const float magnitudeScale = 1.0f / static_cast<float>(fftSize);
  auto magnitudeBufferData = magnitudeArray_->span();

  for (int i = 0; i < magnitudeArray_->getSize(); i++) {
    auto scalarMagnitude = std::abs(complexData_[i]) * magnitudeScale;
    magnitudeBufferData[i] = smoothingTimeConstant_ * magnitudeBufferData[i] +
        (1 - smoothingTimeConstant_) * scalarMagnitude;
  }
}

void AnalyserNode::initializeWindowData(int fftSize) {
  windowData_ = std::make_unique<DSPAudioArray>(fftSize);
  auto data = windowData_->span();
  auto size = windowData_->getSize();

  const auto invSizeMinusOne = 1.0f / static_cast<float>(size - 1);
  const auto alpha = 2.0f * std::numbers::pi_v<float> * invSizeMinusOne;

  for (size_t i = 0; i < size; ++i) {
    const auto phase = alpha * static_cast<float>(i);
    // 4*PI*x is just 2 * (2*PI*x)
    const auto window = 0.42f - 0.50f * std::cos(phase) + 0.08f * std::cos(2.0f * phase);
    data[i] = window;
  }
}

} // namespace audioapi
