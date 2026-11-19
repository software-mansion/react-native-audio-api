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
      framesUntilSnapshot_(static_cast<std::ptrdiff_t>(snapshotFrames())),
      busy_(std::make_shared<std::atomic<bool>>(false)) {
  recentFrames_ = std::make_unique<audioapi::CircularDSPAudioArray>(snapshotFrames());
  if (domain_ == WorkletNodeDomain::FrequencyDomain) {
    fftInput_ = std::make_unique<audioapi::DSPAudioArray>(snapshotFrames());
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
      numFrames == 0) {
    return;
  }

  // Frames older than the newest window would be overwritten within this push anyway.
  const size_t framesToKeep = std::min(numFrames, snapshotFrames());
  recentFrames_->push_back(mono + numFrames - framesToKeep, framesToKeep, true);
  framesUntilSnapshot_ -= static_cast<std::ptrdiff_t>(numFrames);

  if (framesUntilSnapshot_ > 0 || busy_->load(std::memory_order_acquire)) {
    return;
  }

  const auto &channelBuffer = runner_.channelView()->channelBuffer(0);
  if (domain_ == WorkletNodeDomain::TimeDomain) {
    recentFrames_->pop_back(channelBuffer->begin(), bufferLength_, 0, true);
  } else {
    recentFrames_->pop_back(*fftInput_, snapshotFrames(), 0, true);
    spectrumAnalyser_->analyze(*fftInput_, getSmoothingTimeConstant());
    channelBuffer->copy(spectrumAnalyser_->getMagnitudeData(), 0, 0, bufferLength_);
  }
  // The next one is due a whole number of periods after this one was, however late this push
  // took it; any that fell due while the callback was busy are skipped, not caught up on.
  const auto period = static_cast<std::ptrdiff_t>(bufferLength_);
  framesUntilSnapshot_ = period - (-framesUntilSnapshot_ % period);
  dispatchToUI();
}

void UIWorkletSnapshots::reset() {
  framesUntilSnapshot_ = static_cast<std::ptrdiff_t>(snapshotFrames());
}

void UIWorkletSnapshots::deactivate() {
  runner_.deactivate();
}

void UIWorkletSnapshots::dispatchToUI() {
  // push() already saw busy_ false, and only this thread sets it true.
  busy_->store(true, std::memory_order_relaxed);
  runner_.call(sampleRate_, [busy = busy_] { busy->store(false, std::memory_order_release); });
}

} // namespace audioworklets
