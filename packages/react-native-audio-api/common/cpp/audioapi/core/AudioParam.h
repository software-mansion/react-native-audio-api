#pragma once

#include <audioapi/core/types/ParamChangeEventType.h>
#include <audioapi/core/utils/ParamChangeEvent.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/AudioParamEventQueue.h>

#include <cstddef>
#include <utility>
#include <memory>
#include <vector>
#include <unordered_set>
#include <audioapi/utils/CrossThreadEventScheduler.hpp>

namespace audioapi {

class AudioParam {
 public:
  explicit AudioParam(float defaultValue, float minValue, float maxValue, BaseAudioContext *context);

  /// JS-Thread only methods
  /// These methods are called only from HostObjects invoked on the JS thread.

  // JS-Thread only
  [[nodiscard]] inline float getValue() const noexcept {
    return value_;
  }

  // JS-Thread only
  [[nodiscard]] inline float getDefaultValue() const noexcept {
    return defaultValue_;
  }

  // JS-Thread only
  [[nodiscard]] inline float getMinValue() const noexcept {
    return minValue_;
  }

  // JS-Thread only
  [[nodiscard]] inline float getMaxValue() const noexcept {
    return maxValue_;
  }

  // JS-Thread only
  inline void setValue(float value) {
    value_ = std::clamp(value, minValue_, maxValue_);
  }

  // JS-Thread only
  void setValueAtTime(float value, double startTime);

  // JS-Thread only
  void linearRampToValueAtTime(float value, double endTime);

  // JS-Thread only
  void exponentialRampToValueAtTime(float value, double endTime);

  // JS-Thread only
  void setTargetAtTime(float target, double startTime, double timeConstant);

  // JS-Thread only
  void setValueCurveAtTime(
      std::shared_ptr<std::vector<float>> values,
      size_t length,
      double startTime,
      double duration);

  // JS-Thread only
  void cancelScheduledValues(double cancelTime);

  // JS-Thread only
  void cancelAndHoldAtTime(double cancelTime);


  /// Audio-Thread only methods
  /// These methods are called only from the Audio rendering thread.

  // Audio-Thread only (indirectly through AudioNode::connectParam by AudioNodeManager)
  void addInputNode(AudioNode* node);

  // Audio-Thread only (indirectly through AudioNode::disconnectParam by AudioNodeManager)
  void removeInputNode(AudioNode* node);

  // Audio-Thread only
  std::shared_ptr<AudioBus> processARateParam(int framesToProcess, double time);

  // Audio-Thread only
  float processKRateParam(int framesToProcess, double time);

 private:
  // Core parameter state
  BaseAudioContext *context_;
  float value_;
  float defaultValue_;
  float minValue_;
  float maxValue_;

  AudioParamEventQueue eventsQueue_;
  CrossThreadEventScheduler<AudioParam> eventScheduler_;

  // Current automation state (cached for performance)
  double startTime_;
  double endTime_;
  float startValue_;
  float endValue_;
  std::function<float(double, double, float, float, double)> calculateValue_;

  // Input modulation system
  std::vector<AudioNode *> inputNodes_;
  std::shared_ptr<AudioBus> audioBus_;
  std::vector<std::shared_ptr<AudioBus>> inputBuses_;

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

  /// @brief Process all scheduled events.
  inline void processScheduledEvents() noexcept(noexcept(eventScheduler_.processAllEvents(*this))) {
    eventScheduler_.processAllEvents(*this);
  }

  /// @brief Update the parameter queue with a new event.
  /// @param event The new event to add to the queue.
  /// @note Handles connecting start value of the new event to the end value of the previous event.
  inline void updateQueue(ParamChangeEvent &&event) {
    eventsQueue_.pushBack(std::move(event));
  }
  float getValueAtTime(double time);
  void processInputs(const std::shared_ptr<AudioBus>& outputBus, int framesToProcess, bool checkIsAlreadyProcessed);
  void mixInputsBuses(const std::shared_ptr<AudioBus>& processingBus);
  std::shared_ptr<AudioBus> calculateInputs(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess);
};

} // namespace audioapi
