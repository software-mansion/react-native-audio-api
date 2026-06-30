#pragma once

#include <audioapi/events/EventCaller.hpp>

#include <atomic>
#include <cstdint>
#include <memory>

namespace audioapi {

/// @brief Wrapper around EventCaller that dispatches position changed events
/// with interval throttling and optional flush-on-seek behavior.
class PositionChangedDispatcher {
 public:
  PositionChangedDispatcher(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      int intervalInFrames,
      bool shouldFlush = false);

  void assignCallbackId(uint64_t callbackId) noexcept;
  [[nodiscard]] uint64_t getCallbackId() const noexcept;
  [[nodiscard]] bool hasCallback() const noexcept;

  void setIntervalInFrames(int intervalInFrames);
  void setIntervalMs(int intervalInMs, float sampleRate);
  void requestFlush();

  void advance(int framesPlayed, double position);

 private:
  void tryDispatch(double position, bool forceFlush);

  EventCaller<AudioEvent::POSITION_CHANGED> positionChangedEvent_;
  std::atomic<bool> shouldFlush_;
  int intervalInFrames_;
  int accumulatedFrames_ = 0;
};

} // namespace audioapi
