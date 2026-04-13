#pragma once

#ifdef __OBJC__ // when compiled as Objective-C
#import <NativeAudioPlayer.h>
#else  // when compiled as C++
typedef struct objc_object NativeAudioPlayer;
typedef struct objc_object AudioBufferList;
#endif // __OBJC__

#include <audioapi/utils/AudioBuffer.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
namespace audioapi {

class AudioContext;

class IOSAudioPlayer {
 public:
  IOSAudioPlayer(
      const std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> &renderAudio,
      float sampleRate,
      int channelCount);
  ~IOSAudioPlayer();

  bool start();
  void stop();
  bool resume();
  void suspend();
  void cleanup();

  bool isRunning() const;

 private:
  void clearPendingSaved();
  /// Audio-thread only. Always pulls the graph in steps of RENDER_QUANTUM_SIZE; if the system
  /// buffer size is not a multiple of 128, the unused tail of the last quantum is kept (max 128
  /// frames) and played at the start of the next callback.
  void deliverOutputBuffers(AudioBufferList *outputData, int numFrames);

  std::shared_ptr<DSPAudioBuffer> audioBuffer_;
  NativeAudioPlayer *audioPlayer_;
  std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> renderAudio_;
  int channelCount_;
  std::atomic<bool> isRunning_;
  /// Set from main thread on start/resume; consumed on audio thread to drop stale pending audio.
  std::atomic<bool> flushOverflowNextPull_{false};
  /// Frames valid at the front of each `pendingSaved_[ch]` (0 … RENDER_QUANTUM_SIZE).
  int pendingSavedCount_{0};
  DSPAudioBuffer pendingSaved_;
};

} // namespace audioapi
