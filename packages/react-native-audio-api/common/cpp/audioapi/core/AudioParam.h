#pragma once

#include <audioapi/core/utils/ParamChangeEvent.h>
#include <audioapi/core/types/ParamChangeEventType.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/core/AudioNode.h>

#include <deque>
#include <memory>
#include <vector>
#include <unordered_set>
#include <cstddef>
#include <mutex>

namespace audioapi {

class AudioParam {
 public:
  explicit AudioParam(float defaultValue, float minValue, float maxValue, BaseAudioContext *context);

  [[nodiscard]] float getValue() const;
  float getValueAtTime(double time);
  [[nodiscard]] float getDefaultValue() const;
  [[nodiscard]] float getMinValue() const;
  [[nodiscard]] float getMaxValue() const;

  void setValue(float value);

  void setValueAtTime(float value, double startTime);
  void linearRampToValueAtTime(float value, double endTime);
  void exponentialRampToValueAtTime(float value, double endTime);
  void setTargetAtTime(float target, double startTime, double timeConstant);
  void setValueCurveAtTime(
      const float *values,
      size_t length,
      double startTime,
      double duration);
  void cancelScheduledValues(double cancelTime);
  void cancelAndHoldAtTime(double cancelTime);
  void addInputNode(AudioNode* node);
  void removeInputNode(AudioNode* node);
  std::shared_ptr<AudioBus> processARateParam(int framesToProcess, double time);
  float processKRateParam(int framesToProcess, double time);

 private:
  float value_;
  float defaultValue_;
  float minValue_;
  float maxValue_;
  BaseAudioContext *context_;
  std::deque<ParamChangeEvent> eventsQueue_;
  std::unordered_set<AudioNode *> inputNodes_;
  std::shared_ptr<AudioBus> audioBus_;
  std::mutex queueLock_;

  double startTime_;
  double endTime_;
  float startValue_;
  float endValue_;
  std::function<float(double, double, float, float, double)> calculateValue_;
  std::vector<std::shared_ptr<AudioBus>> inputBuses_ = {};

  std::mutex &getQueueLock();
  double getQueueEndTime();
  float getQueueEndValue();
  void updateQueue(ParamChangeEvent &event);
  void processInputs(const std::shared_ptr<AudioBus>& outputBus, int framesToProcess, bool checkIsAlreadyProcessed);
  void mixInputsBuses(const std::shared_ptr<AudioBus>& processingBus);
  std::shared_ptr<AudioBus> calculateInputs(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess);
};

} // namespace audioapi
