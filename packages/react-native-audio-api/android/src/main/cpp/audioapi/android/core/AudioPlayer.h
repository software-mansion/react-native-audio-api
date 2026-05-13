#pragma once

#include <oboe/Oboe.h>
#include <cassert>
#include <functional>
#include <memory>

#include <audioapi/android/core/NativeAudioPlayer.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

namespace audioapi {

using namespace oboe;

class AudioContext;

class AudioPlayer : public AudioStreamDataCallback,
                    public AudioStreamErrorCallback,
                    public std::enable_shared_from_this<AudioPlayer> {
 public:
  friend class AudioContext;
  AudioPlayer(
      const std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> &renderAudio,
      float sampleRate,
      int channelCount);

  ~AudioPlayer() override {
    nativeAudioPlayer_.release();
    cleanup();
  }

  bool start();
  void stop();
  bool resume();
  void suspend();
  void cleanup();

  [[nodiscard]] bool isRunning() const;

  DataCallbackResult onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames)
      override;

  void onErrorAfterClose(AudioStream * /* audioStream */, Result /* error */) override;

 private:
  std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> renderAudio_;
  std::shared_ptr<AudioStream> mStream_;
  std::shared_ptr<DSPAudioBuffer> buffer_;
  bool isInitialized_ = false;
  float sampleRate_;
  int channelCount_;
  std::atomic<bool> isRunning_;

  bool openAudioStream();

  facebook::jni::global_ref<NativeAudioPlayer> nativeAudioPlayer_;
};

} // namespace audioapi
