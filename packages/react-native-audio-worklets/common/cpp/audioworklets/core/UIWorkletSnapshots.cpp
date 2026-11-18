#include <audioworklets/core/UIWorkletSnapshots.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace audioworklets {

UIWorkletSnapshots::UIWorkletSnapshots(
    UIWorkletsRunner runner,
    WorkletNodeDomain domain,
    const WorkletNodeOptions &options)
    : runner_(std::move(runner)),
      domain_(domain),
      bufferLength_(options.bufferLength),
      smoothingTimeConstant_(options.smoothingTimeConstant),
      busy_(std::make_shared<std::atomic<bool>>(false)) {
  accumulator_ = std::make_unique<audioapi::DSPAudioArray>(snapshotFrames());
  if (domain_ == WorkletNodeDomain::FrequencyDomain) {
    spectrumAnalyser_ =
        std::make_unique<audioapi::dsp::SpectrumAnalyser>(static_cast<int>(snapshotFrames()));
  }
  runner_.createChannelView(bufferLength_);
}

UIWorkletSnapshots::~UIWorkletSnapshots() {
  runner_.deactivate();
}

size_t UIWorkletSnapshots::snapshotFrames() const {
  return domain_ == WorkletNodeDomain::FrequencyDomain ? bufferLength_ * 2 : bufferLength_;
}

void UIWorkletSnapshots::push(const float *mono, size_t numFrames) {
  if (!runner_.isActive() || runner_.channelView() == nullptr || mono == nullptr ||
      numFrames == 0 || busy_->load(std::memory_order_acquire)) {
    return;
  }

  const size_t framesToCopy = std::min(numFrames, snapshotFrames() - framesFilled_);
  accumulator_->copy(mono, 0, framesFilled_, framesToCopy);
  framesFilled_ += framesToCopy;

  if (framesFilled_ == snapshotFrames()) {
    const auto &channelBuffer = runner_.channelView()->channelBuffer(0);
    if (domain_ == WorkletNodeDomain::TimeDomain) {
      channelBuffer->copy(*accumulator_, 0, 0, bufferLength_);
    } else {
      spectrumAnalyser_->analyze(*accumulator_, getSmoothingTimeConstant());
      channelBuffer->copy(spectrumAnalyser_->getMagnitudeData(), 0, 0, bufferLength_);
    }
    dispatchToUI();
  }
}

void UIWorkletSnapshots::reset() {
  framesFilled_ = 0;
}

void UIWorkletSnapshots::deactivate() {
  runner_.deactivate();
}

void UIWorkletSnapshots::dispatchToUI() {
  framesFilled_ = 0;

  // push() already saw busy_ false, and only this thread sets it true.
  busy_->store(true, std::memory_order_relaxed);
  runner_.call(sampleRate_, [busy = busy_] { busy->store(false, std::memory_order_release); });
}

} // namespace audioworklets
