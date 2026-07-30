#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/GeneralizedAudioParam.h>
#include <audioapi/core/types/ParamEventType.h>
#include <audioapi/core/utils/param/ParamRenderQueue.h>
#include <audioapi/core/utils/param/RenderParamEvent.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <audioapi/utils/CrossThreadEventScheduler.hpp>
#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

namespace audioapi {

/// @brief A Web Audio API parameter that accepts graph modulation.
///
/// A @c BridgeNode writes modulation into @c inputBuffer_. Processing adds that
/// modulation to the intrinsic automation value, clamps the final result, and
/// clears the buffer so disconnected inputs cannot leave stale modulation.
/// Calls served from the base-class cache return before consuming the buffer.
class AudioParam : public GeneralizedAudioParam {
 public:
  explicit AudioParam(
      float defaultValue,
      float minValue,
      float maxValue,
      const std::shared_ptr<BaseAudioContext> &context);

  /// @note JS Thread only
  /// @return the cached value from last @c processKRateParam() / last sample of last
  /// @c processARateParam().
  [[nodiscard]] float getValue() const noexcept {
    return value_.load(std::memory_order_relaxed);
  }

  [[nodiscard]] float getDefaultValue() const noexcept {
    return defaultValue_;
  }

  void setValue(float value) {
    value_.store(clampToNominalRange(value), std::memory_order_release);
  }

  /// @note Audio Thread only
  void setValueAtTime(float value, double startTime);

  /// @note Audio Thread only
  void linearRampToValueAtTime(float value, double endTime);

  /// @note Audio Thread only
  void exponentialRampToValueAtTime(float value, double endTime);

  /// @note Audio Thread only
  void setTargetAtTime(float target, double startTime, double timeConstant);

  /// @note Audio Thread only
  void setValueCurveAtTime(
      const std::shared_ptr<AudioArray> &values,
      size_t length,
      double startTime,
      double duration);

  /// @note Audio Thread only
  void cancelScheduledValues(double cancelTime);

  /// @note Audio Thread only
  void cancelAndHoldAtTime(double cancelTime);

  template <typename F>
  bool scheduleAudioEvent(F &&event) noexcept
    requires(std::is_invocable_r_v<void, std::decay_t<F>, BaseAudioContext &>)
  {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      return context->scheduleAudioEvent(std::forward<F>(event));
    }

    return false;
  }

  /// @brief Returns the input buffer where BridgeNode stores mixed modulation signals.
  /// @note Audio Thread only
  [[nodiscard]] std::shared_ptr<DSPAudioBuffer> getInputBuffer() const {
    return inputBuffer_;
  }

  /// @note Audio Thread only
  /// @note Add BridgeNode modulation from @c inputBuffer_ on top of the automated value,
  /// clamp, cache, then zero @c inputBuffer_. Idempotent for the same args.
  std::shared_ptr<DSPAudioBuffer> processARateParam(int framesToProcess, double time) final;

  /// @note Audio Thread only
  /// @note Add BridgeNode modulation from @c inputBuffer_ on top of the automated value,
  /// clamp, cache, then zero @c inputBuffer_. Idempotent for the same args.
  float processKRateParam(double time) final;

 private:
  std::atomic<float> value_;
  float defaultValue_;

  ParamRenderQueue eventRenderQueue_;

  // Input modulation buffer - filled by BridgeNode during graph processing.
  std::shared_ptr<DSPAudioBuffer> inputBuffer_;

  /// @brief Update the parameter queue with a new event.
  /// @param event The new event to add to the queue.
  /// @note Resolves the event's startValue and startTime based on neighboring events,
  // and adjusts neighboring events to maintain the invariant of non-overlapping events in the queue.
  void updateQueue(RenderParamEvent &&event) {
    eventRenderQueue_.push(std::move(event));
  }

  /// @note Audio Thread only. @c eventRenderQueue_ is unsynchronized and must
  /// not be accessed from another thread. When automation is active, this also
  /// updates @c value_ so JavaScript @c getValue() observes the latest intrinsic
  /// sample; BridgeNode modulation is applied only by @c processKRateParam /
  /// @c processARateParam.
  float getValueAtTimeUnmodulated(double time);
};

} // namespace audioapi
