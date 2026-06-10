#pragma once

#include <audioapi/events/OnPositionChangedDriver.h>
#include <audioapi/utils/Macros.h>

#include <cstdint>
#include <memory>

namespace audioapi {

class OnPositionChangedNode {
 public:
  explicit OnPositionChangedNode(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      int intervalInFrames,
      bool shouldFlush = false)
      : onPositionChangedDriver_(audioEventHandlerRegistry, intervalInFrames, shouldFlush) {}
  virtual ~OnPositionChangedNode() = default;
  DELETE_COPY_AND_MOVE(OnPositionChangedNode);

  void assignOnPositionChangedCallbackId(uint64_t callbackId) {
    onPositionChangedDriver_.assignCallbackId(callbackId);
  }

  [[nodiscard]] uint64_t getOnPositionChangedCallbackId() const {
    return onPositionChangedDriver_.getCallbackId();
  }

  void unregisterOnPositionChangedCallback(uint64_t callbackId) {
    onPositionChangedDriver_.unregisterCallback(callbackId);
  }

 protected:
  OnPositionChangedDriver onPositionChangedDriver_;
};

} // namespace audioapi
