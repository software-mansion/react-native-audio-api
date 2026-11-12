#pragma once

#include <audioapi/utils/Macros.h>

#include <atomic>
#include <cstdint>

namespace audioapi {

class AudioRecorder;

/// @brief Lets the platform input's audio thread reach the recorder only while the recorder
/// allows it, so the recorder can change what that thread reads without the audio thread ever
/// taking a lock.
class RecorderInputGate {
 public:
  explicit RecorderInputGate(AudioRecorder *recorder) : recorder_(recorder) {}
  DELETE_COPY_AND_MOVE(RecorderInputGate);
  ~RecorderInputGate() = default;

  /// Audio thread. Forwards the frames to the recorder while armed, drops them otherwise.
  /// @p channels holds @p channelCount pointers, each to @p numFrames float32 samples, valid only
  /// for the call.
  void deliver(const float *const *channels, int channelCount, int numFrames);

  /// Recorder control thread.
  void arm();

  /// Recorder control thread. Returns once no callback is inside the recorder, and none will
  /// enter it before the next arm(). Waits at most for the one callback in flight, so it must not
  /// be called from the audio thread, which would wait for itself.
  void disarmAndWait();

 private:
  AudioRecorder *const recorder_;
  std::atomic<bool> armed_{false};
  std::atomic<uint32_t> callbacksInFlight_{0};
};

} // namespace audioapi
