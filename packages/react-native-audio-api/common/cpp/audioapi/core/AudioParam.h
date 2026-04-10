#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/types/ParamEventType.h>
#include <audioapi/core/utils/param/ParamRenderQueue.h>
#include <audioapi/core/utils/param/RenderParamEvent.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <audioapi/utils/CrossThreadEventScheduler.hpp>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

class AudioParam {
 public:
  explicit AudioParam(
      float defaultValue,
      float minValue,
      float maxValue,
      const std::shared_ptr<BaseAudioContext> &context);

  [[nodiscard]] float getValue() const noexcept {
    return value_.load(std::memory_order_relaxed);
  }

  [[nodiscard]] float getDefaultValue() const noexcept {
    return defaultValue_;
  }

  [[nodiscard]] float getMinValue() const noexcept {
    return minValue_;
  }

  [[nodiscard]] float getMaxValue() const noexcept {
    return maxValue_;
  }

  void setValue(float value) {
    value_.store(std::clamp(value, minValue_, maxValue_), std::memory_order_release);
  }

  /// @note JS Thread only
  [[nodiscard]] double getCurrentTime() const noexcept {
    if (auto context = context_.lock()) {
      return context->getCurrentTime();
    }
    return 0.0;
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

  /// Audio-Thread only methods
  /// These methods are called only from the Audio rendering thread.

  /// @note Audio Thread only
  void addInputNode(AudioNode *node);

  /// @note Audio Thread only
  void removeInputNode(AudioNode *node);

  /// @note Audio Thread only
  std::shared_ptr<DSPAudioBuffer> processARateParam(int framesToProcess, double time);

  /// @note Audio Thread only
  float processKRateParam(int framesToProcess, double time);

 private:
  // Core parameter state
  std::weak_ptr<BaseAudioContext> context_;
  std::atomic<float> value_;
  float defaultValue_;
  float minValue_;
  float maxValue_;

  ParamRenderQueue eventRenderQueue_;

  // Input modulation system
  std::vector<AudioNode *> inputNodes_;
  std::shared_ptr<DSPAudioBuffer> audioBuffer_;
  std::vector<std::shared_ptr<DSPAudioBuffer>> inputBuffers_;

  /// @brief Update the parameter queue with a new event.
  /// @param event The new event to add to the queue.
  /// @note Resolves the event's startValue and startTime based on neighboring events,
  // and adjusts neighboring events to maintain the invariant of non-overlapping events in the queue.
  void updateQueue(RenderParamEvent &&event) {
    eventRenderQueue_.push(std::move(event));
  }

  float getValueAtTime(double time);
  void processInputs(
      const std::shared_ptr<DSPAudioBuffer> &outputBuffer,
      int framesToProcess,
      bool checkIsAlreadyProcessed);
  void mixInputsBuffers(const std::shared_ptr<DSPAudioBuffer> &processingBuffer);
  std::shared_ptr<DSPAudioBuffer> calculateInputs(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess);
};

} // namespace audioapi
