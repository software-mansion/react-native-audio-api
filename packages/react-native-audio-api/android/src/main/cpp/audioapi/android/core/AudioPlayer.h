#pragma once

#include <audioapi/utils/Macros.h>
#include <oboe/Oboe.h>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <mutex>

#include <audioapi/core/CommonPlayer.h>

namespace audioapi {

using namespace oboe;

class AudioPlayer : public CommonPlayer,
                    public AudioStreamDataCallback,
                    public AudioStreamErrorCallback,
                    public std::enable_shared_from_this<AudioPlayer> {
 public:
  using CommonPlayer::CommonPlayer;

  ~AudioPlayer() override {
    cleanup();
  }

  DELETE_COPY_AND_MOVE(AudioPlayer);

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
  mutable std::recursive_mutex streamMutex_;
  std::atomic<bool> isInitialized_{false};
  /// Updated on the audio thread from each Oboe callback `numFrames`.
  std::atomic<int32_t> lastCallbackFrameCount_{0};

  bool openAudioStream();
  bool rebuildStream();
};

} // namespace audioapi
