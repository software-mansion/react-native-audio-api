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

  /// Allocates the snapshot buffer and pre-builds a stable mono `Float32Array` view on
  /// the UI worklet runtime. Must be called once before the first `call`.
  void createChannelView(size_t frameCount);

  /// Native buffer filled on the audio thread; view is read on the UI thread.
  [[nodiscard]] const std::shared_ptr<AudioChannelViews> &channelView() const {
    return job_->channelView;
  }

  /// @brief Schedules the worklet to run on the UI thread with the pre-built
  /// `Float32Array` view from `createChannelView`.
  /// @param onComplete Invoked on the UI thread once the worklet returns.
  /// @note Audio Thread only.
  void call(std::function<void()> onComplete) const;

 private:
  struct UIWorkletJob {
    std::shared_ptr<std::atomic<bool>> alive;
    std::weak_ptr<worklets::WorkletRuntime> uiRuntime;
    std::weak_ptr<worklets::UIScheduler> uiScheduler;
    std::shared_ptr<worklets::Serializable> serializableWorklet;
    std::shared_ptr<AudioChannelViews> channelView;
    std::function<void()> onComplete;
  };

  static void runUIWorkletJob(const std::shared_ptr<UIWorkletJob> &job);

  /// Shared by the runner and scheduled UI lambdas; only one job may be in flight.
  std::shared_ptr<UIWorkletJob> job_;
};

} // namespace audioworklets
