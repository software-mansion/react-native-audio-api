#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <worklets/Compat/StableApi.h>

#include <jsi/jsi.h>

#include <functional>
#include <memory>
#include <vector>

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
 * All state is held in shared pointers so a scheduled job stays valid even if
 * the owning node is destroyed before the job runs on the UI thread.
 */
class UIWorkletsRunner {
 public:
  UIWorkletsRunner(
      std::shared_ptr<worklets::WorkletRuntime> uiRuntime,
      std::shared_ptr<worklets::UIScheduler> uiScheduler,
      std::shared_ptr<worklets::Serializable> serializableWorklet);

  [[nodiscard]] bool isValid() const {
    return uiRuntime_ != nullptr && uiScheduler_ != nullptr && serializableWorklet_ != nullptr;
  }

  /// @brief Schedules the worklet to run on the UI thread with the given audio
  /// data. The channel buffer pool is captured (kept alive) by the scheduled job;
  /// the JSI arrays are built on the UI runtime, inside the UI thread.
  /// @param channels Shared, fixed-size pool of per-channel snapshot buffers.
  /// @param channelCount Number of channels to expose to the worklet.
  /// @param onComplete Invoked on the UI thread once the worklet returns.
  /// @note Audio Thread only.
  void invokeOnUI(
      std::shared_ptr<std::vector<std::shared_ptr<audioapi::AudioArrayBuffer>>> channels,
      size_t channelCount,
      std::function<void()> onComplete) const;

 private:
  std::shared_ptr<worklets::WorkletRuntime> uiRuntime_;
  std::shared_ptr<worklets::UIScheduler> uiScheduler_;
  std::shared_ptr<worklets::Serializable> serializableWorklet_;
};

} // namespace audioworklets
