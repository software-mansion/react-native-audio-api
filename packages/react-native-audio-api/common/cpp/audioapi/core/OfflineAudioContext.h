#pragma once

#include "BaseAudioContext.h"

#include <mutex>
#include <map>

namespace audioapi {

using OfflineAudioContextSuspendCallback = std::function<void()>;
using OfflineAudioContextResultCallback = std::function<void(std::shared_ptr<AudioBuffer>)>;

class OfflineAudioContext : public BaseAudioContext {
 public:
  explicit OfflineAudioContext(float sampleRate, int32_t numFrames);
  ~OfflineAudioContext() override;

  void resume();
  void suspend(double when, OfflineAudioContextSuspendCallback callback);

  void startRendering(OfflineAudioContextResultCallback callback);

 private:
  bool isRenderingStarted_;
  std::unordered_map<int32_t, OfflineAudioContextSuspendCallback> scheduledSuspends_;
  OfflineAudioContextResultCallback resultCallback_;

  int32_t numFrames_;
  int32_t currentSampleFrame_;
\
  std::shared_ptr<AudioBus> resultBus_;

  std::mutex stateLock_;

  void resumeRendering();

};

} // namespace audioapi