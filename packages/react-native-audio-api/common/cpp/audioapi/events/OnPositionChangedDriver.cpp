#include <audioapi/events/OnPositionChangedDriver.h>

#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/AudioEventPayload.h>

#include <algorithm>
#include <memory>

namespace audioapi {

OnPositionChangedDriver::OnPositionChangedDriver(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    int intervalInFrames,
    bool shouldFlush)
    : audioEventHandlerRegistry_(audioEventHandlerRegistry),
      shouldFlush_(shouldFlush),
      intervalInFrames_(intervalInFrames) {}

void OnPositionChangedDriver::assignCallbackId(uint64_t callbackId) {
  callbackId_.store(callbackId, std::memory_order_release);
}

uint64_t OnPositionChangedDriver::getCallbackId() const {
  return callbackId_.load(std::memory_order_acquire);
}

void OnPositionChangedDriver::unregisterCallback(uint64_t callbackId) {
  audioEventHandlerRegistry_->unregisterHandler(AudioEvent::POSITION_CHANGED, callbackId);
}

void OnPositionChangedDriver::setIntervalInFrames(int intervalInFrames) {
  intervalInFrames_ = std::max(0, intervalInFrames);
}

void OnPositionChangedDriver::setIntervalMs(int intervalInMs, float sampleRate) {
  //NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
  setIntervalInFrames(static_cast<int>(sampleRate * static_cast<float>(intervalInMs) / 1000.0f));
}

void OnPositionChangedDriver::requestFlush() {
  shouldFlush_.store(true, std::memory_order_release);
}

void OnPositionChangedDriver::advance(int framesPlayed, double position) {
  tryDispatch(position, shouldFlush_.load(std::memory_order_acquire));
  accumulatedFrames_ += framesPlayed;
}

void OnPositionChangedDriver::tryDispatch(double position, bool forceFlush) {
  const auto callbackId = callbackId_.load(std::memory_order_acquire);
  if (callbackId != 0 && (forceFlush || accumulatedFrames_ > intervalInFrames_)) {
    audioEventHandlerRegistry_->dispatchEvent(
        AudioEvent::POSITION_CHANGED, callbackId, DoubleValuePayload{.value = position});

    accumulatedFrames_ = 0;
    if (forceFlush) {
      shouldFlush_.store(false, std::memory_order_release);
    }
  }
}

} // namespace audioapi
