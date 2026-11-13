#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/jsi/ContextPromiseResolver.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <condition_variable>
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

  void resume(std::unique_ptr<ContextStateResolver> promise);
  bool suspend(double when, std::unique_ptr<ContextStateResolver> promise);
  void startRendering(std::unique_ptr<ContextRenderResolver> promise);

  /// @brief Always null, so convolver tails are computed inline on the render thread.
  std::shared_ptr<ConvolverThreadPool> getConvolverThreadPool() override;

 private:
  std::unordered_map<size_t, std::unique_ptr<ContextStateResolver>> scheduledSuspends_;
  std::unique_ptr<ContextRenderResolver> resultPromise_;
  bool renderingStarted_{false};

  const size_t length_;
  size_t currentSampleFrame_;

  std::shared_ptr<DSPAudioBuffer> audioBuffer_;
  std::shared_ptr<AudioBuffer> resultBuffer_;

  /// The only render worker, spawned by `startRendering` and parked on `resumed_` across every
  /// scheduled suspend. Owned: it captures `this`, so the destructor must be able to stop and
  /// join it before members are torn down.
  std::thread renderThread_;
  /// Set by the destructor to end the render loop. Guarded by `driverMutex_`.
  bool stopRendering_{false};

  /// Wakes the parked render thread; paired with `driverMutex_`.
  std::condition_variable resumed_;
  /// The accepted resume() waiting for the render thread to wake up and resolve it.
  /// Guarded by `driverMutex_`.
  std::unique_ptr<ContextStateResolver> pendingResume_;

  void renderUntilFinished();
  /// @note `driverMutex_` must be held.
  void renderQuantum();
  /// @brief Parks the render thread until `resume` or the destructor wakes it. A resume has
  /// its promise resolved here; a destructor wake-up leaves `stopRendering_` for the caller.
  /// @param driverLock Held lock on `driverMutex_`, released while parked.
  void waitForResume(std::unique_lock<std::mutex> &driverLock);

  bool isDriverRunning() const override;
};

} // namespace audioapi
