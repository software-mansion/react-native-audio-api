#pragma once

#include "BaseAudioContext.h"

#include <mutex>
#include <map>
#include <unordered_map>
#include <memory>

namespace audioapi {

using OfflineAudioContextSuspendCallback = std::function<void()>;
using OfflineAudioContextResultCallback = std::function<void(std::shared_ptr<AudioBuffer>)>;

class OfflineAudioContext : public BaseAudioContext {
 public:
  explicit OfflineAudioContext(int numberOfChannels, size_t length, float sampleRate);
  ~OfflineAudioContext() override;

  void resume();
  void suspend(double when, OfflineAudioContextSuspendCallback callback);

  void startRendering(OfflineAudioContextResultCallback callback);

 private:
  bool isRenderingStarted_;
  std::unordered_map<int32_t, OfflineAudioContextSuspendCallback> scheduledSuspends_;
  OfflineAudioContextResultCallback resultCallback_;

  int32_t length_;
  int numberOfChannels_;
  int32_t currentSampleFrame_;

  std::shared_ptr<AudioBus> resultBus_;

  std::mutex stateLock_;

  void resumeRendering();
};

} // namespace audioapi
