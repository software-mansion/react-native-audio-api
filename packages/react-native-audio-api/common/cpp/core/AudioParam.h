#pragma once

#include <memory>
#include <set>
#include <vector>

#include "ParamChangeEvent.h"
#include "ParamChangeEventType.h"

namespace audioapi {

class BaseAudioContext;

class AudioParam {
 public:
  explicit AudioParam(
      BaseAudioContext *context,
      float defaultValue,
      float minValue,
      float maxValue);

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
      int length,
      double startTime,
      double duration);
  void cancelScheduledValues(double cancelTime);
  void cancelAndHoldAtTime(double cancelTime);

 private:
  float value_;
  float defaultValue_;
  float minValue_;
  float maxValue_;
  BaseAudioContext *context_;
  std::set<ParamChangeEvent> eventsQueue_;

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
