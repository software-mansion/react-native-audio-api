#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/types/ParamChangeEventType.h>
#include <audioapi/core/utils/AudioParamEventQueue.h>
#include <audioapi/core/utils/ParamChangeEvent.hpp>
#include <audioapi/utils/AudioBuffer.h>

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

  [[nodiscard]] inline float getValue() const noexcept {
    return value_.load(std::memory_order_relaxed);
  }

  [[nodiscard]] inline float getDefaultValue() const noexcept {
    return defaultValue_;
  }

  [[nodiscard]] inline float getMinValue() const noexcept {
    return minValue_;
  }

  [[nodiscard]] inline float getMaxValue() const noexcept {
    return maxValue_;
  }

  inline void setValue(float value) {
    value_.store(std::clamp(value, minValue_, maxValue_), std::memory_order_release);
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

  template <
      typename F,
      typename = std::enable_if_t<std::is_invocable_r_v<void, std::decay_t<F>, BaseAudioContext &>>>
  bool inline scheduleAudioEvent(F &&event) noexcept {
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
  std::shared_ptr<AudioBuffer> processARateParam(int framesToProcess, double time);

  /// @note Audio Thread only
  float processKRateParam(int framesToProcess, double time);

 private:
  // Core parameter state
  std::weak_ptr<BaseAudioContext> context_;
  std::atomic<float> value_;
  float defaultValue_;
  float minValue_;
  float maxValue_;

  AudioParamEventQueue eventsQueue_;

  // Current automation state (cached for performance)
  double startTime_;
  double endTime_;
  float startValue_;
  float endValue_;
  std::function<float(double, double, float, float, double)> calculateValue_;

  // Input modulation system
  std::vector<AudioNode *> inputNodes_;
  std::shared_ptr<AudioBuffer> audioBuffer_;
  std::vector<std::shared_ptr<AudioBuffer>> inputBuffers_;

  /// @brief Get the end time of the parameter queue.
  /// @return The end time of the parameter queue or last endTime_ if queue is empty.
  inline double getQueueEndTime() const noexcept {
    if (eventsQueue_.isEmpty()) {
      return endTime_;
    }
    return eventsQueue_.back().getEndTime();
  }

  /// @brief Get the end value of the parameter queue.
  /// @return The end value of the parameter queue or last endValue_ if queue is empty.
  inline float getQueueEndValue() const noexcept {
    if (eventsQueue_.isEmpty()) {
      return endValue_;
    }
    return eventsQueue_.back().getEndValue();
  }

  /// @brief Update the parameter queue with a new event.
  /// @param event The new event to add to the queue.
  /// @note Handles connecting start value of the new event to the end value of the previous event.
  inline void updateQueue(ParamChangeEvent &&event) {
    eventsQueue_.pushBack(std::move(event));
  }
  float getValueAtTime(double time);
  void processInputs(
      const std::shared_ptr<AudioBuffer> &outputBuffer,
      int framesToProcess,
      bool checkIsAlreadyProcessed);
  void mixInputsBuffers(const std::shared_ptr<AudioBuffer> &processingBuffer);
  std::shared_ptr<AudioBuffer> calculateInputs(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess);
};

} // namespace audioapi
