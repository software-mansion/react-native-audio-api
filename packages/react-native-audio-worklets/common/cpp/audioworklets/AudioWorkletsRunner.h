#pragma once

#include <worklets/Compat/StableApi.h>
#include <worklets/WorkletRuntime/WorkletRuntime.h>

#include <audioworklets/utils/AudioChannelViews.h>

#include <jsi/jsi.h>

#include <array>
#include <cstddef>
#include <memory>
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

  [[nodiscard]] bool isActive() const {
    return workletInitialized_ && unsafeRuntimePtr_ != nullptr;
  }

  /// @brief Invokes the cached worklet on the audio worklet runtime.
  /// @note Audio Thread only. Caller must check `isActive()` first.
  template <typename... Args>
  jsi::Value callUnsafe(Args &&...args) const {
    return getUnsafeWorklet().call(*unsafeRuntimePtr_, std::forward<Args>(args)...);
  }

  /// @brief Allocates channel buffers and pre-builds stable `Float32Array[]` views on
  /// the audio worklet runtime.
  /// @param frameCount Number of frames per channel (view length).
  /// @param channelCount Number of channel slots in the pool.
  /// @note Must be called once before the first `callUnsafe`.
  [[nodiscard]] std::shared_ptr<AudioChannelViews> createChannelViews(
      size_t frameCount,
      size_t channelCount);

 private:
  std::weak_ptr<worklets::WorkletRuntime> weakRuntime_;
  jsi::Runtime *unsafeRuntimePtr_ = nullptr;

  /// @brief Placement storage for the deserialized worklet `jsi::Function`.
  alignas(jsi::Function) std::array<std::byte, sizeof(jsi::Function)> unsafeWorklet_{};

  bool workletInitialized_ = false;

  void destroyCachedWorklet();

  [[nodiscard]] const jsi::Function &getUnsafeWorklet() const {
    return *reinterpret_cast<const jsi::Function *>(unsafeWorklet_.data());
  }
};

} // namespace audioworklets
