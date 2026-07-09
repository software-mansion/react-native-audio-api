#pragma once

#include <worklets/Compat/StableApi.h>
#include <worklets/WorkletRuntime/WorkletRuntime.h>

#include <audioworklets/AudioChannelViews.h>

#include <jsi/jsi.h>

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace audioworklets {

using namespace facebook;

/**
 * Invokes a serializable worklet synchronously on the audio WorkletRuntime from
 * the audio thread.
 *
 * Used by `WorkletSourceNode` and `WorkletProcessingNode`. Channel views are
 * pre-built via `createChannelViews` and passed to the worklet on each render
 * quantum without per-call JSI allocation.
 */
class AudioWorkletsRunner {
 public:
  AudioWorkletsRunner(
      std::weak_ptr<worklets::WorkletRuntime> weakRuntime,
      const std::shared_ptr<worklets::Serializable> &serializableWorklet);

  AudioWorkletsRunner(const AudioWorkletsRunner &) = delete;
  AudioWorkletsRunner &operator=(const AudioWorkletsRunner &) = delete;
  AudioWorkletsRunner(AudioWorkletsRunner &&other) noexcept;
  AudioWorkletsRunner &operator=(AudioWorkletsRunner &&other) noexcept;
  ~AudioWorkletsRunner();

  /// @brief Invokes the worklet without acquiring the runtime mutex.
  /// @note Audio Thread only. Caller must ensure the runtime is valid.
  template <typename... Args>
  jsi::Value callUnsafe(Args &&...args) {
    return getUnsafeWorklet().call(*unsafeRuntimePtr_, std::forward<Args>(args)...);
  }

  /// @brief Invokes the worklet synchronously on the audio worklet runtime.
  /// @returns `std::nullopt` when the runtime or worklet is unavailable.
  /// @note Audio Thread only.
  template <typename... Args>
  std::optional<jsi::Value> call(Args &&...args) const {
    auto strongRuntime = weakRuntime_.lock();
    if (strongRuntime == nullptr || !workletInitialized_) {
      return std::nullopt;
    }

    return strongRuntime->runSync(getUnsafeWorklet(), std::forward<Args>(args)...);
  }

  /// @brief Runs a job synchronously on the audio worklet runtime.
  std::optional<jsi::Value> executeOnRuntimeSync(
      const std::function<jsi::Value(jsi::Runtime &)> &&job) const noexcept(noexcept(job));

  /// @brief Allocates channel buffers and pre-builds stable `Float32Array[]` views on
  /// the audio worklet runtime.
  /// @param frameCount Number of frames per channel (view length).
  /// @param channelCount Number of channel slots in the pool.
  /// @note Must be called once before the first `call`.
  [[nodiscard]] std::shared_ptr<AudioChannelViews> createChannelViews(
      size_t frameCount,
      size_t channelCount);

 private:
  std::weak_ptr<worklets::WorkletRuntime> weakRuntime_;
  jsi::Runtime *unsafeRuntimePtr_ = nullptr;

  /// @brief Placement storage for the deserialized worklet `jsi::Function`.
  alignas(jsi::Function) std::array<std::byte, sizeof(jsi::Function)> unsafeWorklet_{};

  bool workletInitialized_ = false;

  [[nodiscard]] const jsi::Function &getUnsafeWorklet() const {
    return *reinterpret_cast<const jsi::Function *>(unsafeWorklet_.data());
  }
};

} // namespace audioworklets
