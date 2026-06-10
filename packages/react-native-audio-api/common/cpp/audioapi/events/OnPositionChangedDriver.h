#pragma once

#include <audioapi/events/IAudioEventHandlerRegistry.h>

#include <atomic>
#include <cstdint>
#include <memory>

namespace audioapi {

class OnPositionChangedDriver {
 public:
  explicit OnPositionChangedDriver(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      int intervalInFrames,
      bool shouldFlush = false);

  void assignCallbackId(uint64_t callbackId);
  [[nodiscard]] uint64_t getCallbackId() const;
  void unregisterCallback(uint64_t callbackId);

  void setIntervalInFrames(int intervalInFrames);
  void setIntervalMs(int intervalInMs, float sampleRate);
  void requestFlush();

  void advance(int framesPlayed, double position);

 private:
  void tryDispatch(double position, bool forceFlush);

  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  std::atomic<uint64_t> callbackId_{0};
  std::atomic<bool> shouldFlush_{false};
  int intervalInFrames_;
  int accumulatedFrames_ = 0;
};

} // namespace audioapi
