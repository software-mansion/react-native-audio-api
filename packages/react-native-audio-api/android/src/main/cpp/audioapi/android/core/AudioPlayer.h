#pragma once

#include <oboe/Oboe.h>
#include <functional>
#include <memory>
#include <cassert>

namespace audioapi {

using namespace oboe;

class AudioContext;
class AudioBus;

class AudioPlayer : public AudioStreamDataCallback {
 public:
  AudioPlayer(
      const std::function<void(std::shared_ptr<AudioBus>, int)> &renderAudio,
      float sampleRate);

  [[nodiscard]] float getSampleRate() const;
  void start();
  void stop();
  void resume();
  void suspend();

  DataCallbackResult onAudioReady(
      AudioStream *oboeStream,
      void *audioData,
      int32_t numFrames) override;

 private:
  std::function<void(std::shared_ptr<AudioBus>, int)> renderAudio_;
  std::shared_ptr<AudioStream> mStream_;
  std::shared_ptr<AudioBus> mBus_;
  bool isInitialized_ = false;
  float sampleRate_;
  int channelCount_;
};

} // namespace audioapi
