#pragma once

#include <audioapi/utils/FatFunction.hpp>
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <cstdint>
#include <utility>

namespace audioapi {

/// @brief Lets the audio thread reach the recorder only while armed, without taking a lock.
class RecorderInputGate {
 public:
  /// @p channels holds @p channelCount pointers to @p numFrames float32 samples each.
  using Sink = FatFunction<
      sizeof(void *),
      void(const float *const *channels, int channelCount, int numFrames)>;

  explicit RecorderInputGate(Sink sink) : sink_(std::move(sink)) {}
  DELETE_COPY_AND_MOVE(RecorderInputGate);
  ~RecorderInputGate() = default;

  /// @brief Taken first in a platform callback: disarmAndWait() waits for its whole body. Empty
  /// when disarmed at entry; the callback then returns without touching anything.
  struct Pass {
    explicit Pass(RecorderInputGate &gate);
    DELETE_COPY_AND_MOVE(Pass);
    ~Pass();

    explicit operator bool() const {
      return isAdmitted;
    }

    /// Passes @p frames on to the Sink when admitted.
    template <typename... Frames>
    void deliver(Frames &&...frames) const {
      if (isAdmitted) {
        gate.sink_(std::forward<Frames>(frames)...);
      }
    }

    RecorderInputGate &gate;
    bool isAdmitted;
  };

  [[nodiscard]] Pass enter();

  void arm();
  void disarmAndWait();

 private:
  const Sink sink_;
  std::atomic<bool> armed_{false};
  std::atomic<uint32_t> callbacksInFlight_{0};
};

} // namespace audioapi
