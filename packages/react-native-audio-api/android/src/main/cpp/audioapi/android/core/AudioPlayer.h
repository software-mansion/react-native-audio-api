#pragma once

#include <oboe/Oboe.h>
#include <cassert>
#include <functional>
#include <memory>

namespace audioapi {

using namespace oboe;

class AudioContext;
class AudioBus;

class AudioPlayer : public AudioStreamDataCallback, AudioStreamErrorCallback {
 public:
  AudioPlayer(
      const std::function<void(std::shared_ptr<AudioBus>, int)> &renderAudio,
      float sampleRate,
      int channelCount);

  bool start();
  void stop();
  bool resume();
  void suspend();
  void cleanup();

  bool isRunning() const;

  DataCallbackResult onAudioReady(
      AudioStream *oboeStream,
      void *audioData,
      int32_t numFrames) override;

  void onErrorAfterClose(AudioStream * /* audioStream */, Result /* error */)
      override;

 private:
  std::function<void(std::shared_ptr<AudioBus>, int)> renderAudio_;
  std::shared_ptr<AudioStream> mStream_;
  std::shared_ptr<AudioBus> mBus_;
  bool isInitialized_ = false;
  float sampleRate_;
  int channelCount_;

  bool openAudioStream();
};

} // namespace audioapi
