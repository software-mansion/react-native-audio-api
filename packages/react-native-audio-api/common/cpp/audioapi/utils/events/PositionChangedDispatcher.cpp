#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/utils/events/PositionChangedDispatcher.h>

#include <algorithm>
#include <memory>

namespace audioapi {

PositionChangedDispatcher::PositionChangedDispatcher(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    int intervalInFrames,
    bool shouldFlush)
    : positionChangedEvent_(audioEventHandlerRegistry),
      shouldFlush_(shouldFlush),
      intervalInFrames_(intervalInFrames) {}

void PositionChangedDispatcher::assignCallbackId(uint64_t callbackId) noexcept {
  positionChangedEvent_.assignCallbackId(callbackId);
}

uint64_t PositionChangedDispatcher::getCallbackId() const noexcept {
  return positionChangedEvent_.getCallbackId();
}

bool PositionChangedDispatcher::hasCallback() const noexcept {
  return positionChangedEvent_.hasCallback();
}

void PositionChangedDispatcher::setIntervalInFrames(int intervalInFrames) {
  intervalInFrames_ = std::max(0, intervalInFrames);
}

void PositionChangedDispatcher::setIntervalMs(int intervalInMs, float sampleRate) {
  setIntervalInFrames(static_cast<int>(dsp::timeToSampleFrame(intervalInMs * 0.001, sampleRate)));
}

void PositionChangedDispatcher::requestFlush() {
  shouldFlush_.store(true, std::memory_order_release);
}

void PositionChangedDispatcher::advance(int framesPlayed, double position) {
  tryDispatch(position, shouldFlush_.load(std::memory_order_acquire));
  accumulatedFrames_ += framesPlayed;
}

void PositionChangedDispatcher::tryDispatch(double position, bool forceFlush) {
  if (hasCallback() && (forceFlush || accumulatedFrames_ > intervalInFrames_)) {
    positionChangedEvent_.dispatchFromAudioThread(DoubleValuePayload{.value = position});

    accumulatedFrames_ = 0;
    if (forceFlush) {
      shouldFlush_.store(false, std::memory_order_release);
    }
  }
}

} // namespace audioapi
