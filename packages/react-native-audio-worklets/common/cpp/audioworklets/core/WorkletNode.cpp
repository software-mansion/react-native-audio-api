#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/core/WorkletNode.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace audioworklets {

WorkletNode::WorkletNode(
    const std::shared_ptr<audioapi::BaseAudioContext> &context,
    UIWorkletsRunner workletRunner,
    size_t bufferLength)
    : audioapi::AudioNode(context),
      workletRunner_(std::move(workletRunner)),
      bufferLength_(bufferLength),
      busy_(std::make_shared<std::atomic<bool>>(false)) {
  workletRunner_.createChannelViews(
      bufferLength_, static_cast<size_t>(audioapi::MAX_CHANNEL_COUNT));
}

WorkletNode::~WorkletNode() {
  workletRunner_.deactivate();
}

void WorkletNode::processNode(int framesToProcess) {
  const auto &channelViews = workletRunner_.channelViews();
  if (!workletRunner_.isActive() || channelViews == nullptr) {
    return;
  }

  if (framesToProcess <= 0) {
    return;
  }

  if (busy_->load(std::memory_order_acquire)) {
    return;
  }

  const auto frameCount = static_cast<size_t>(framesToProcess);
  const auto channelCount = audioBuffer_->getNumberOfChannels();

  if (channelCount == 0) {
    return;
  }

  const size_t channelsToCopy =
      std::min(channelCount, static_cast<size_t>(audioapi::MAX_CHANNEL_COUNT));

  const size_t remaining = bufferLength_ - framesFilled_;
  if (remaining == 0) {
    return;
  }

  const size_t framesToCopy = std::min(frameCount, remaining);

  for (size_t ch = 0; ch < channelsToCopy; ++ch) {
    channelViews->channelBuffer(ch)->copy(
        *audioBuffer_->getChannel(ch), 0, framesFilled_, framesToCopy);
  }

  framesFilled_ += framesToCopy;

  if (framesFilled_ >= bufferLength_) {
    dispatchToUI(channelsToCopy);
  }
}

void WorkletNode::dispatchToUI(size_t channelCount) {
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
  workletRunner_.call(channelCount, [busy]() { busy->store(false, std::memory_order_release); });

  framesFilled_ = 0;
}

} // namespace audioworklets
