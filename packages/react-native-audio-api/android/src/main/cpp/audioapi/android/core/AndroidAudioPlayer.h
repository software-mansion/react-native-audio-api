#pragma once

#include <audioapi/utils/Macros.h>
#include <oboe/Oboe.h>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <mutex>

#include <audioapi/core/AudioPlayer.h>
#include <audioapi/core/types/AudioContextOptions.h>

namespace audioapi {

using namespace oboe;

class AndroidAudioPlayer : public AudioPlayer,
                           public AudioStreamDataCallback,
                           public AudioStreamErrorCallback,
                           public std::enable_shared_from_this<AndroidAudioPlayer> {
 public:
  AndroidAudioPlayer(
      const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
      float sampleRate,
      int channelCount,
      std::atomic<uint32_t> &currentRenders,
      std::weak_ptr<AudioContext> context,
      std::mutex *driverMutex,
      AudioContextLatencyHint latencyHint,
      AndroidOutputProfile outputProfile);

  ~AndroidAudioPlayer() override {
    cleanup();
  }

  DELETE_COPY_AND_MOVE(AndroidAudioPlayer);

  bool start() override;
  void stop() override;
  bool resume() override;
  void suspend() override;
  void cleanup() override;

  [[nodiscard]] bool isRunning() const override;

  [[nodiscard]] double getBaseLatency() const override;
  [[nodiscard]] double getOutputLatency() const override;

  DataCallbackResult onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames)
      override;

  void onErrorAfterClose(AudioStream *audioStream, Result error) override;

 private:
  std::shared_ptr<AudioStream> mStream_;
  /// `Locked` methods require it to be held by the caller and must not call the public locking methods.
  /// When both locks are needed, `driverMutex_` is taken first.
  mutable std::mutex streamMutex_;
  std::atomic<bool> isInitialized_{false};
  /// Updated on the audio thread from each Oboe callback `numFrames`.
  std::atomic<int32_t> lastCallbackFrameCount_{0};
  AndroidOutputProfile outputProfile_;

  bool openAudioStreamLocked();
  bool rebuildStreamLocked();
  bool startStreamLocked();
  void cleanupLocked();
  [[nodiscard]] bool isRunningLocked() const;
};

} // namespace audioapi
