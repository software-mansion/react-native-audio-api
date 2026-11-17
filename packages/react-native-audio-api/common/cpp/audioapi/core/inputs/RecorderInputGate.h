#pragma once

#include <audioapi/utils/Macros.h>

#include <atomic>
#include <cstdint>

namespace audioapi {

class AudioRecorder;

/// @brief Lets the audio thread reach the recorder only while armed, without taking a lock.
class RecorderInputGate {
 public:
  explicit RecorderInputGate(AudioRecorder *recorder) : recorder_(recorder) {}
  DELETE_COPY_AND_MOVE(RecorderInputGate);
  ~RecorderInputGate() = default;

  /// @brief Taken first in a platform callback: disarmAndWait() waits for its whole body. Empty
  /// when disarmed at entry; the callback then returns without touching anything.
  class Pass {
   public:
    DELETE_COPY_AND_MOVE(Pass);
    ~Pass();

    explicit operator bool() const {
      return isAdmitted_;
    }

    /// @p channels holds @p channelCount pointers to @p numFrames float32 samples each.
    void deliver(const float *const *channels, int channelCount, int numFrames) const;

   private:
    friend class RecorderInputGate;
    explicit Pass(RecorderInputGate &gate);

    RecorderInputGate &gate_;
    bool isAdmitted_;
  };

  [[nodiscard]] Pass enter();

  void arm();
  void disarmAndWait();

 private:
  AudioRecorder *const recorder_;
  std::atomic<bool> armed_{false};
  std::atomic<uint32_t> callbacksInFlight_{0};
};

} // namespace audioapi
