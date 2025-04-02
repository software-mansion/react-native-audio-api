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
  void suspend(double when, const OfflineAudioContextSuspendCallback& callback);

  void startRendering(OfflineAudioContextResultCallback callback);

 private:
  std::mutex mutex_;
  bool isRenderingStarted_;

  std::unordered_map<size_t, OfflineAudioContextSuspendCallback> scheduledSuspends_;
  OfflineAudioContextResultCallback resultCallback_;

  size_t length_;
  int numberOfChannels_;
  size_t currentSampleFrame_;

  std::shared_ptr<AudioBus> resultBus_;

  void resumeRendering();
};

} // namespace audioapi
