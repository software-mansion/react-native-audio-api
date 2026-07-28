#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/jsi/ContextPromiseResolver.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <memory>
#include <unordered_map>

namespace audioapi {

using OfflineAudioContextResultPromise = ContextPromiseResolver<std::shared_ptr<AudioBuffer>>;

class OfflineAudioContext : public BaseAudioContext {
 public:
  explicit OfflineAudioContext(
      int numberOfChannels,
      size_t length,
      float sampleRate,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry);
  ~OfflineAudioContext() override = default;
  DELETE_COPY_AND_MOVE(OfflineAudioContext);

  void resume(const std::shared_ptr<ContextPromiseResolver<>> &promise);
  bool suspend(double when, const std::shared_ptr<ContextPromiseResolver<>> &promise);
  void startRendering(const std::shared_ptr<OfflineAudioContextResultPromise> &promise);

 private:
  std::unordered_map<size_t, std::shared_ptr<ContextPromiseResolver<>>> scheduledSuspends_;
  std::shared_ptr<OfflineAudioContextResultPromise> resultPromise_;
  bool renderingStarted_{false};

  const size_t length_;
  size_t currentSampleFrame_;

  std::shared_ptr<DSPAudioBuffer> audioBuffer_;
  std::shared_ptr<AudioBuffer> resultBuffer_;

  void renderAudio(const std::shared_ptr<ContextPromiseResolver<>> &resumePromise);

  bool isDriverRunning() const override;
};

} // namespace audioapi
