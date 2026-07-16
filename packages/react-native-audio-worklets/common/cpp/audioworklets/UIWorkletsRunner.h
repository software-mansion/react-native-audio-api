#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/utils/AudioChannelViews.h>
#include <worklets/Compat/StableApi.h>

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>

namespace audioworklets {

using namespace facebook;

/**
 * Drives a worklet on the shared UI worklet runtime for UI-animation nodes.
 *
 * Built on the stable C++ APIs of both dependencies:
 *  - `audioapi/compatibility/StableAPI.h` for audio buffers,
 *  - `worklets/Compat/StableApi.h` for UI-thread scheduling and worklet invocation.
 *
 * Model: the UI worklet runtime is owned/driven by the UI thread (it is the
 * same runtime react-native-reanimated animates on). We must never touch its
 * JSI runtime from the audio thread, so audio data is snapshotted on the audio
 * thread and the actual worklet invocation is marshalled onto the UI thread via
 * `scheduleOnUI`. This is fire-and-forget: UI animation does not need a result
 * and must not block the audio thread.
 *
 * Scheduled jobs capture a pre-allocated `UIWorkletJob` shared_ptr so the
 * `scheduleOnUI` closure stays small. Only one job may be in flight at a time.
 */
class UIWorkletsRunner {
 public:
  UIWorkletsRunner(
      const std::shared_ptr<worklets::WorkletRuntime> &uiRuntime,
      const std::shared_ptr<worklets::UIScheduler> &uiScheduler,
      std::shared_ptr<worklets::Serializable> serializableWorklet);

  /// Stops scheduling new UI jobs and causes in-flight lambdas to no-op.
  /// JSI views are released on the UI scheduler after any in-flight job completes.
  /// Safe to call multiple times.
  void deactivate();

  [[nodiscard]] bool isActive() const;

  /// Allocates channel buffers and pre-builds stable `Float32Array[]` views on
  /// the UI worklet runtime. Must be called once before the first `call`.
  void createChannelViews(size_t frameCount, size_t channelCount);

  /// Native buffer pool filled on the audio thread; views are read on the UI thread.
  [[nodiscard]] const std::shared_ptr<AudioChannelViews> &channelViews() const {
    return job_->channelViews;
  }

  /// @brief Schedules the worklet to run on the UI thread with pre-built channel
  /// views from `createChannelViews`.
  /// @param channelCount Number of channels to expose to the worklet.
  /// @param onComplete Invoked on the UI thread once the worklet returns.
  /// @note Audio Thread only.
  void call(size_t channelCount, std::function<void()> onComplete) const;

 private:
  struct UIWorkletJob {
    std::shared_ptr<std::atomic<bool>> alive;
    std::weak_ptr<worklets::WorkletRuntime> uiRuntime;
    std::weak_ptr<worklets::UIScheduler> uiScheduler;
    std::shared_ptr<worklets::Serializable> serializableWorklet;
    std::shared_ptr<AudioChannelViews> channelViews;
    size_t channelCount{0};
    std::function<void()> onComplete;
  };

  static void runUIWorkletJob(const std::shared_ptr<UIWorkletJob> &job);

  /// Shared by the runner and scheduled UI lambdas; only one job may be in flight.
  std::shared_ptr<UIWorkletJob> job_;
};

} // namespace audioworklets
