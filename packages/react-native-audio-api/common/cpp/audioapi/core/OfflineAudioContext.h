#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/jsi/ContextPromiseResolver.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>

namespace audioapi {

class OfflineAudioContext : public BaseAudioContext {
 public:
  explicit OfflineAudioContext(
      int numberOfChannels,
      size_t length,
      float sampleRate,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry);
  ~OfflineAudioContext() override;
  DELETE_COPY_AND_MOVE(OfflineAudioContext);

  void resume(const std::shared_ptr<ContextPromiseResolver<void>> &promise);
  bool suspend(double when, const std::shared_ptr<ContextPromiseResolver<void>> &promise);
  void startRendering(const std::shared_ptr<OfflineAudioContextResultPromise> &promise);

 private:
  std::unordered_map<size_t, std::shared_ptr<ContextPromiseResolver<void>>> scheduledSuspends_;
  std::shared_ptr<OfflineAudioContextResultPromise> resultPromise_;
  bool renderingStarted_{false};

  const size_t length_;
  size_t currentSampleFrame_;

  std::shared_ptr<DSPAudioBuffer> audioBuffer_;
  std::shared_ptr<AudioBuffer> resultBuffer_;

  /// Render worker. Owned: it captures `this`, so the destructor must be able
  /// to stop and join it before members are torn down.
  std::thread renderThread_;
  std::atomic<bool> stopRendering_{false};

  void renderAudio(const std::shared_ptr<ContextPromiseResolver<void>> &resumePromise);

  bool isDriverRunning() const override;
};

} // namespace audioapi
