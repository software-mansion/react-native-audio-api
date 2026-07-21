#pragma once

#include <audioapi/utils/Macros.h>
#include <oboe/Oboe.h>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

#include <audioapi/android/core/NativeAudioPlayer.hpp>
#include <audioapi/core/CommonPlayer.h>
#include <audioapi/utils/AudioBuffer.hpp>

namespace audioapi {

using namespace oboe;

class AudioContext;

class AudioPlayer : public CommonPlayer,
                    public AudioStreamDataCallback,
                    public AudioStreamErrorCallback,
                    public std::enable_shared_from_this<AudioPlayer> {
 public:
  AudioPlayer(
      const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
      float sampleRate,
      int channelCount,
      std::mutex *driverMutex,
      const std::shared_ptr<AudioContext> &context,
      std::atomic<uint32_t> &currentRenders);

  ~AudioPlayer() override {
    nativeAudioPlayer_.release();
    cleanup();
  }

  DELETE_COPY_AND_MOVE(AudioPlayer);

  bool start() override;
  void stop() override;
  bool resume() override;
  void suspend() override;
  void cleanup() override;

  [[nodiscard]] bool isRunning() const override;

  [[nodiscard]] double getBaseLatency() const;
  [[nodiscard]] double getOutputLatency() const;

  DataCallbackResult onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames)
      override;

  void onErrorAfterClose(AudioStream *audioStream, Result error) override;

 private:
  std::function<void(DSPAudioBuffer *, int)> renderAudio_;
  std::atomic<uint32_t> &currentRenders_;
  std::shared_ptr<AudioStream> mStream_;
  mutable std::recursive_mutex streamMutex_;
  std::shared_ptr<DSPAudioBuffer> buffer_;
  std::atomic<bool> isInitialized_{false};
  float sampleRate_;
  int channelCount_;
  std::atomic<bool> isRunning_;
  /// Updated on the audio thread from each Oboe callback `numFrames`.
  std::atomic<int32_t> lastCallbackFrameCount_{0};
  std::mutex *driverMutex_;
  std::weak_ptr<AudioContext> context_;

  bool openAudioStream();

  facebook::jni::global_ref<NativeAudioPlayer> nativeAudioPlayer_;
};

} // namespace audioapi
