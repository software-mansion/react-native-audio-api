#pragma once

#include "BaseAudioContext.h"
#include <audioapi/core/utils/worklets/SafeIncludes.h>

#include <mutex>
#include <map>
#include <unordered_map>
#include <memory>

namespace audioapi {

using OfflineAudioContextSuspendCallback = std::function<void()>;
using OfflineAudioContextResultCallback = std::function<void(std::shared_ptr<AudioBuffer>)>;

class OfflineAudioContext : public BaseAudioContext {
 public:
  explicit OfflineAudioContext(int numberOfChannels, size_t length, float sampleRate, const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry, const RuntimeRegistry &runtimeRegistry);
  ~OfflineAudioContext() override;

  void resume();
  void suspend(double when, const OfflineAudioContextSuspendCallback& callback);

  void startRendering(OfflineAudioContextResultCallback callback);

 private:
  std::mutex mutex_;

  std::unordered_map<size_t, OfflineAudioContextSuspendCallback> scheduledSuspends_;
  OfflineAudioContextResultCallback resultCallback_;

  size_t length_;
  int numberOfChannels_;
  size_t currentSampleFrame_;

  std::shared_ptr<AudioBus> resultBus_;

  void renderAudio();

  bool isDriverRunning() const override;
};

} // namespace audioapi
