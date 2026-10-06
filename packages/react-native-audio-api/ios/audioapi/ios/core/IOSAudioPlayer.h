#pragma once

#ifdef __OBJC__ // when compiled as Objective-C
#import <NativeAudioPlayer.h>
#else  // when compiled as C++
typedef struct objc_object NativeAudioPlayer;
typedef struct objc_object AudioBufferList;
#endif // __OBJC__

#include <audioapi/core/AudioPlayer.h>
#include <audioapi/core/types/AudioContextLatencyHint.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

namespace audioapi {

class IOSAudioPlayer : public AudioPlayer {
 public:
  using AudioPlayer::AudioPlayer;
  ~IOSAudioPlayer() override;

  DELETE_COPY_AND_MOVE(IOSAudioPlayer);

  bool start() override;
  void stop() override;
  bool resume() override;
  void suspend() override;
  void cleanup() override;

  [[nodiscard]] bool isRunning() const override;

  [[nodiscard]] double getBaseLatency() const override;
  [[nodiscard]] double getOutputLatency() const override;

 private:
  void clearPendingSaved();
  /// @note Audio Thread only
  /// Always pulls the graph in steps of RENDER_QUANTUM_SIZE; if the system
  /// buffer size is not a multiple of 128, the unused tail of the last quantum
  /// is kept (max 128 frames) and played at the start of the next callback.
  void deliverOutputBuffers(AudioBufferList *outputData, int numFrames);

  /// Builds the native player and points its render and stream-failure blocks at this object.
  /// Runs as a member initializer, before the members below exist: the blocks only capture
  /// `this` and must not be invoked until construction has finished.
  NativeAudioPlayer *createNativePlayer();

  NativeAudioPlayer *audioPlayer_ = createNativePlayer();
  /// Set from main thread on start/resume; consumed on audio thread to drop stale pending audio.
  std::atomic<bool> flushOverflowNextPull_{false};
  /// Frames valid at the front of each `pendingSaved_[ch]` (0 … RENDER_QUANTUM_SIZE).
  int pendingSavedCount_{0};
  DSPAudioBuffer pendingSaved_{RENDER_QUANTUM_SIZE, channelCount_, sampleRate_};
};

} // namespace audioapi
