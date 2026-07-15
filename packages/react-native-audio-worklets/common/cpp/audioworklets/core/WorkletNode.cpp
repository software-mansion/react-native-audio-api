#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/core/WorkletNode.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace audioworklets {

WorkletNode::WorkletNode(
    const std::shared_ptr<audioapi::BaseAudioContext> &context,
    UIWorkletsRunner workletRunner,
    size_t bufferLength)
    : audioapi::AudioNode(context),
      downMixBuffer_(
          std::make_unique<audioapi::DSPAudioBuffer>(
              audioapi::RENDER_QUANTUM_SIZE,
              1,
              context->getSampleRate())),
      workletRunner_(std::move(workletRunner)),
      bufferLength_(bufferLength),
      busy_(std::make_shared<std::atomic<bool>>(false)) {
  setProcessableState(GraphObject::PROCESSABLE_STATE::ALWAYS_PROCESSABLE);
  workletRunner_.createChannelView(bufferLength_);
}

WorkletNode::~WorkletNode() {
  workletRunner_.deactivate();
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

  downMixBuffer_->copy(*audioBuffer_);

  const auto frameCount = static_cast<size_t>(framesToProcess);

  const size_t remaining = bufferLength_ - framesFilled_;
  if (remaining == 0) {
    return;
  }

  const size_t framesToCopy = std::min(frameCount, remaining);

  channelView->channelBuffer(0)->copy(
      *downMixBuffer_->getChannel(0), 0, framesFilled_, framesToCopy);

  framesFilled_ += framesToCopy;

  if (framesFilled_ >= bufferLength_) {
    dispatchToUI();
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
