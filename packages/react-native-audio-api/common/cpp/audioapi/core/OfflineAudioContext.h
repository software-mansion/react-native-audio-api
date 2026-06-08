#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <memory>
#include <mutex>
#include <unordered_map>

namespace audioapi {

using OfflineAudioContextSuspendCallback = std::function<void()>;
using OfflineAudioContextResultCallback = std::function<void(std::shared_ptr<AudioBuffer>)>;

class OfflineAudioContext : public BaseAudioContext {
 public:
  explicit OfflineAudioContext(
      int numberOfChannels,
      size_t length,
      float sampleRate,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const RuntimeRegistry &runtimeRegistry);
  ~OfflineAudioContext() override;
  DELETE_COPY_AND_MOVE(OfflineAudioContext);

  /// @note JS Thread only
  void resume();

  /// @note JS Thread only
  void suspend(double when, const OfflineAudioContextSuspendCallback &callback);

  /// @note JS Thread only
  void startRendering(OfflineAudioContextResultCallback callback);

 private:
  std::mutex mutex_;

  std::unordered_map<size_t, OfflineAudioContextSuspendCallback> scheduledSuspends_;
  OfflineAudioContextResultCallback resultCallback_;

  const size_t length_;
  size_t currentSampleFrame_;

  std::shared_ptr<DSPAudioBuffer> audioBuffer_;
  std::shared_ptr<AudioBuffer> resultBuffer_;

  void renderAudio();

  bool isDriverRunning() const override;
};

} // namespace audioapi
