#include <audioapi/utils/events/BufferingStateDispatcher.h>

#include <memory>

namespace audioapi {

BufferingStateDispatcher::BufferingStateDispatcher(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    int startThresholdFrames)
    : bufferingStateChangeEvent_(audioEventHandlerRegistry),
      startThresholdFrames_(startThresholdFrames) {}

void BufferingStateDispatcher::assignCallbackId(uint64_t callbackId) noexcept {
  bufferingStateChangeEvent_.assignCallbackId(callbackId);
}

uint64_t BufferingStateDispatcher::getCallbackId() const noexcept {
  return bufferingStateChangeEvent_.getCallbackId();
}

bool BufferingStateDispatcher::hasCallback() const noexcept {
  return bufferingStateChangeEvent_.hasCallback();
}

bool BufferingStateDispatcher::isBuffering() const noexcept {
  return isBuffering_.load(std::memory_order_acquire);
}

void BufferingStateDispatcher::advance(bool hasData, int framesToProcess) {
  if (!bufferingStateChangeEvent_.hasCallback()) {
    return;
  }

  if (hasData) {
    starvedFrames_ = 0;
    if (isBuffering_.exchange(false, std::memory_order_acq_rel)) {
      bufferingStateChangeEvent_.dispatchFromAudioThread(BoolValuePayload{.value = false});
    }
    return;
  }

  starvedFrames_ += framesToProcess;
  if (!isBuffering_.load(std::memory_order_acquire) && starvedFrames_ >= startThresholdFrames_) {
    isBuffering_.store(true, std::memory_order_release);
    bufferingStateChangeEvent_.dispatchFromAudioThread(BoolValuePayload{.value = true});
  }
}

} // namespace audioapi
