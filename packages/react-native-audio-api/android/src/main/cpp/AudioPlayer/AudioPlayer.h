#pragma once

#include <oboe/Oboe.h>
#include <memory>

namespace audioapi {

using namespace oboe;

class AudioContext;
class AudioBus;

class AudioPlayer : public AudioStreamDataCallback {
 public:
  explicit AudioPlayer(const std::function<void(AudioBus*, int)> &renderAudio);

  int getSampleRate() const;
  int getBufferSizeInFrames() const;
  void start();
  void stop();

  DataCallbackResult onAudioReady(
      AudioStream *oboeStream,
      void *audioData,
      int32_t numFrames) override;

 private:
  std::function<void(AudioBus*, int)> renderAudio_;
  std::shared_ptr<AudioStream> mStream_;
  std::shared_ptr<AudioBus> mBus_;
  bool isInitialized_ = false;
};

} // namespace audioapi
