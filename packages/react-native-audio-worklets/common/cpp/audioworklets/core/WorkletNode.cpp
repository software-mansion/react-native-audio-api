#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/core/WorkletNode.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace audioworklets {

WorkletNode::WorkletNode(
    const std::shared_ptr<audioapi::BaseAudioContext> &context,
    UIWorkletsRunner workletRunner)
    : audioapi::AudioNode(context),
      workletRunner_(std::move(workletRunner)),
      busy_(std::make_shared<std::atomic<bool>>(false)) {
  snapshotBuffers_ = std::make_shared<
      std::vector<std::shared_ptr<audioapi::AudioArrayBuffer>>>(
      static_cast<size_t>(audioapi::MAX_CHANNEL_COUNT));
  for (size_t ch = 0; ch < static_cast<size_t>(audioapi::MAX_CHANNEL_COUNT); ++ch) {
    (*snapshotBuffers_)[ch] =
        std::make_shared<audioapi::AudioArrayBuffer>(audioapi::RENDER_QUANTUM_SIZE);
  }
}

void WorkletNode::processNode(int framesToProcess) {
  if (framesToProcess <= 0) {
    return;
  }

  const size_t frameCount = static_cast<size_t>(framesToProcess);
  const size_t channelCount = audioBuffer_->getNumberOfChannels();

  if (channelCount == 0) {
    return;
  }

  dispatchToUI(frameCount, channelCount);
}

void WorkletNode::dispatchToUI(size_t frameCount, size_t channelCount) {
  if (!workletRunner_.isValid()) {
    return;
  }

  const size_t channelsToCopy =
      std::min(channelCount, static_cast<size_t>(audioapi::MAX_CHANNEL_COUNT));

  // Drop this quantum if the previous UI invocation has not completed yet.
  bool expected = false;
  if (!busy_->compare_exchange_strong(
          expected,
          true,
          std::memory_order_acq_rel,
          std::memory_order_acquire)) {
    return;
  }

  for (size_t ch = 0; ch < channelsToCopy; ++ch) {
    (*snapshotBuffers_)[ch]->copy(*audioBuffer_->getChannel(ch), 0, 0, frameCount);
  }

  std::atomic_thread_fence(std::memory_order_release);

  auto busy = busy_;
  workletRunner_.invokeOnUI(snapshotBuffers_, channelsToCopy, [busy]() {
    busy->store(false, std::memory_order_release);
  });
}

} // namespace audioworklets
