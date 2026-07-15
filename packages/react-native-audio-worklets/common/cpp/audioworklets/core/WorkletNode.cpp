#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/core/WorkletNode.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <utility>
#include <vector>

namespace audioworklets {

size_t WorkletNode::fftSizeForBufferLength(size_t bufferLength) {
  return bufferLength * 2;
}

WorkletNode::WorkletNode(
    const std::shared_ptr<audioapi::BaseAudioContext> &context,
    UIWorkletsRunner workletRunner,
    WorkletNodeDomain domain,
    const WorkletNodeOptions &options)
    : audioapi::AudioNode(context),
      domain_(domain),
      bufferLength_(options.bufferLength),
      smoothingTimeConstant_(options.smoothingTimeConstant),
      downMixBuffer_(
          std::make_unique<audioapi::DSPAudioBuffer>(
              audioapi::RENDER_QUANTUM_SIZE,
              1,
              context->getSampleRate())),
      workletRunner_(std::move(workletRunner)),
      busy_(std::make_shared<std::atomic<bool>>(false)) {
  if (domain == WorkletNodeDomain::TimeDomain) {
    timeDomainAccum_ = std::make_unique<audioapi::DSPAudioArray>(bufferLength_);
  } else {
    initializeFrequencyDomain(static_cast<int>(fftSizeForBufferLength(bufferLength_)));
  }

  setProcessableState(GraphObject::PROCESSABLE_STATE::ALWAYS_PROCESSABLE);
  workletRunner_.createChannelView(bufferLength_);
}

WorkletNode::~WorkletNode() {
  workletRunner_.deactivate();
}

void WorkletNode::setSmoothingTimeConstant(float smoothingTimeConstant) {
  smoothingTimeConstant_.store(smoothingTimeConstant, std::memory_order_release);
}

void WorkletNode::initializeFrequencyDomain(int fftSize) {
  fft_ = std::make_unique<audioapi::dsp::FFT>(fftSize);
  frequencyTimeDomainAccum_ = std::make_unique<audioapi::DSPAudioArray>(fftSize);
  tempArray_ = std::make_unique<audioapi::DSPAudioArray>(fftSize);
  magnitudeArray_ = std::make_unique<audioapi::DSPAudioArray>(fftSize / 2);
  complexData_ = std::vector<std::complex<float>>(fftSize);
  initializeWindowData(fftSize);
}

void WorkletNode::processNode(int framesToProcess) {
  const auto &channelView = workletRunner_.channelView();
  if (!workletRunner_.isActive() || channelView == nullptr) {
    return;
  }

  if (framesToProcess <= 0) {
    return;
  }

  if (busy_->load(std::memory_order_acquire)) {
    return;
  }

  if (domain_ == WorkletNodeDomain::TimeDomain) {
    processTimeDomain(framesToProcess);
    return;
  }

  processFrequencyDomain(framesToProcess);
}

void WorkletNode::processTimeDomain(int framesToProcess) {
  const auto &channelView = workletRunner_.channelView();

  downMixBuffer_->copy(*audioBuffer_);

  const auto frameCount = static_cast<size_t>(framesToProcess);

  const size_t remaining = bufferLength_ - framesFilled_;
  if (remaining == 0) {
    return;
  }

  const size_t framesToCopy = std::min(frameCount, remaining);

  timeDomainAccum_->copy(*downMixBuffer_->getChannel(0), 0, framesFilled_, framesToCopy);

  framesFilled_ += framesToCopy;

  if (framesFilled_ >= bufferLength_) {
    channelView->channelBuffer(0)->copy(*timeDomainAccum_, 0, 0, bufferLength_);
    dispatchToUI();
  }
}

void WorkletNode::processFrequencyDomain(int framesToProcess) {
  const auto &channelView = workletRunner_.channelView();

  downMixBuffer_->copy(*audioBuffer_);

  const auto frameCount = static_cast<size_t>(framesToProcess);
  const auto fftSize = fftSizeForBufferLength(bufferLength_);

  const size_t remaining = fftSize - framesFilled_;
  if (remaining == 0) {
    return;
  }

  const size_t framesToCopy = std::min(frameCount, remaining);

  frequencyTimeDomainAccum_->copy(*downMixBuffer_->getChannel(0), 0, framesFilled_, framesToCopy);

  framesFilled_ += framesToCopy;

  if (framesFilled_ >= fftSize) {
    doFFTAnalysis();
    channelView->channelBuffer(0)->copy(*magnitudeArray_, 0, 0, bufferLength_);
    dispatchToUI();
  }
}

void WorkletNode::doFFTAnalysis() {
  const auto fftSize = fftSizeForBufferLength(bufferLength_);
  const auto smoothingTimeConstant = smoothingTimeConstant_.load(std::memory_order_acquire);

  tempArray_->copy(*frequencyTimeDomainAccum_, 0, 0, fftSize);
  tempArray_->multiply(*windowData_, fftSize);

  fft_->doFFT(*tempArray_, complexData_);

  complexData_[0] = std::complex<float>(complexData_[0].real(), 0);

  const float magnitudeScale = 1.0f / static_cast<float>(fftSize);
  auto magnitudeBufferData = magnitudeArray_->span();

  for (size_t i = 0; i < magnitudeArray_->getSize(); ++i) {
    const auto scalarMagnitude = std::abs(complexData_[i]) * magnitudeScale;
    magnitudeBufferData[i] = smoothingTimeConstant * magnitudeBufferData[i] +
        (1.0f - smoothingTimeConstant) * scalarMagnitude;
  }
}

void WorkletNode::initializeWindowData(int fftSize) {
  windowData_ = std::make_unique<audioapi::DSPAudioArray>(static_cast<size_t>(fftSize));
  auto data = windowData_->span();
  const auto size = windowData_->getSize();

  const auto invSize = 1.0f / static_cast<float>(size);
  const auto alpha = 2.0f * std::numbers::pi_v<float> * invSize;

  for (size_t i = 0; i < size; ++i) {
    const auto phase = alpha * static_cast<float>(i);
    const auto window = 0.42f - 0.50f * std::cos(phase) + 0.08f * std::cos(2.0f * phase);
    data[i] = window;
  }
}

void WorkletNode::processInputs(
    const std::vector<const audioapi::DSPAudioBuffer *> &inputs,
    int numFrames) {
  if (inputs.empty()) {
    getInputBuffer()->zero(0, static_cast<size_t>(numFrames));
    framesFilled_ = 0;
    return;
  }

  audioapi::AudioNode::processInputs(inputs, numFrames);
}

void WorkletNode::dispatchToUI() {
  if (!workletRunner_.isActive()) {
    framesFilled_ = 0;
    return;
  }

  bool expected = false;
  if (!busy_->compare_exchange_strong(
          expected, true, std::memory_order_acq_rel, std::memory_order_acquire)) {
    framesFilled_ = 0;
    return;
  }

  std::atomic_thread_fence(std::memory_order_release);

  auto busy = busy_;
  workletRunner_.call([busy]() { busy->store(false, std::memory_order_release); });

  framesFilled_ = 0;
}

} // namespace audioworklets
