#pragma once

#include <audioapi/core/utils/ParamChangeEvent.h>
#include <audioapi/core/types/ParamChangeEventType.h>

#include <deque>
#include <memory>
#include <vector>
#include <cstddef>

namespace audioapi {

class AudioArray;

class AudioParam {
 public:
  explicit AudioParam(float defaultValue, float minValue, float maxValue);

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
      const std::shared_ptr<AudioArray> &values,
      size_t length,
      double startTime,
      double duration);
  void cancelScheduledValues(double cancelTime);
  void cancelAndHoldAtTime(double cancelTime);

 private:
  float value_;
  float defaultValue_;
  float minValue_;
  float maxValue_;
  std::deque<ParamChangeEvent> eventsQueue_;

  double startTime_;
  double endTime_;
  float startValue_;
  float endValue_;
  std::function<float(double, double, float, float, double)> calculateValue_;

  double getQueueEndTime();
  float getQueueEndValue();
  void updateQueue(ParamChangeEvent &event);
};

} // namespace audioapi
