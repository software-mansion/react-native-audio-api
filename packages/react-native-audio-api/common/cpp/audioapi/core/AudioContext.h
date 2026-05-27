#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <functional>
#include <memory>

namespace audioapi {
#ifdef ANDROID
class AudioPlayer;
#else
class IOSAudioPlayer;
#endif

class AudioContext : public BaseAudioContext {
 public:
  explicit AudioContext(
      float sampleRate,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const RuntimeRegistry &runtimeRegistry);
  ~AudioContext() override;
  DELETE_COPY_AND_MOVE(AudioContext);

  void close();
  bool resume();
  bool suspend();
  bool start();
  void initialize() override;

  std::shared_ptr<MediaElementAudioSourceNode> createMediaElementSource(
      const std::shared_ptr<AudioFileSourceNode> &fileSource);

 private:
#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif
  std::atomic<bool> isInitialized_{false};

  bool isDriverRunning() const override;

  std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> renderAudio();
};

} // namespace audioapi
