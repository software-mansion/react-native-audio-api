#pragma once

#include <audioapi/events/EventCaller.hpp>

#include <atomic>
#include <cstdint>
#include <memory>

namespace audioapi {

/// @brief Debounces render-thread frame starvation into a buffering-state
/// event. A single starved render quantum is normal decode-ahead jitter, not
/// a real stall — only sustained starvation past @p startThresholdFrames is
/// reported. Recovery is reported immediately, with no symmetric debounce.
class BufferingStateDispatcher {
 public:
  BufferingStateDispatcher(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      int startThresholdFrames);

  void assignCallbackId(uint64_t callbackId) noexcept;
  [[nodiscard]] uint64_t getCallbackId() const noexcept;
  [[nodiscard]] bool hasCallback() const noexcept;

  /// @brief True once sustained starvation has been reported and no
  /// recovery has fired yet.
  [[nodiscard]] bool isBuffering() const noexcept;

  /// @brief Call once per render quantum. @p hasData is true when the
  /// quantum had a decoded chunk available (fresh or previously stashed) to
  /// play; false when the decoder daemon has nothing ready yet.
  /// @note Audio thread only.
  void advance(bool hasData, int framesToProcess);

 private:
  EventCaller<AudioEvent::BUFFERING_STATE_CHANGE> bufferingStateChangeEvent_;
  std::atomic<bool> isBuffering_{false};
  int starvedFrames_ = 0;
  const int startThresholdFrames_;
};

} // namespace audioapi
