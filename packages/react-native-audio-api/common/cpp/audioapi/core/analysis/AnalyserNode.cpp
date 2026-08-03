#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/types/NodeOptions.h>

#include <algorithm>
#include <memory>

namespace audioapi {

AnalyserNode::AnalyserNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const AnalyserOptions &options)
    : AudioNode(context, options),
      fftSize_(options.fftSize),
      inputArray_(std::make_unique<CircularDSPAudioArray>(MAX_FFT_SIZE * 2)),
      downMixBuffer_(
          std::make_unique<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())),
      minDecibels_(options.minDecibels),
      maxDecibels_(options.maxDecibels),
      smoothingTimeConstant_(options.smoothingTimeConstant),
      spectrumAnalyser_(options.fftSize) {
  setProcessableState(GraphObject::PROCESSABLE_STATE::ALWAYS_PROCESSABLE);
}

void AnalyserNode::setFFTSize(int fftSize) {
  if (fftSize == fftSize_.load(std::memory_order_acquire)) {
    return;
  }

  spectrumAnalyser_.setFFTSize(fftSize);
  fftSize_.store(fftSize, std::memory_order_release);
}

void AnalyserNode::getFloatFrequencyData(float *data, int length) {
  doFFTAnalysis();

  const auto &magnitudeArray = spectrumAnalyser_.getMagnitudeData();
  length = std::min(static_cast<int>(magnitudeArray.getSize()), length);
  auto magnitudeSpan = magnitudeArray.span();

  for (int i = 0; i < length; i++) {
    data[i] = dsp::linearToDecibels(magnitudeSpan[i]);
  }
}

void AnalyserNode::getByteFrequencyData(uint8_t *data, int length) {
  doFFTAnalysis();

  const auto &magnitudeArray = spectrumAnalyser_.getMagnitudeData();
  auto magnitudeBufferData = magnitudeArray.span();
  length = std::min(static_cast<int>(magnitudeArray.getSize()), length);

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

  spectrumAnalyser_.analyze(frame->timeDomain, smoothingTimeConstant_);
}

} // namespace audioapi
