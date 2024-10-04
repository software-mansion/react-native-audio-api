#pragma once

#include <memory>
#include <set>
#include <vector>

#include "ParamChange.h"

namespace audioapi {

class AudioContext;

class AudioParam {
 public:
  explicit AudioParam(
      AudioContext *context,
      float defaultValue,
      float minValue,
      float maxValue);

  float getValue() const;
  float getValueAtTime(double time);
  void setValue(float value);
  float getDefaultValue() const;
  float getMinValue() const;
  float getMaxValue() const;
  void setValueAtTime(float value, double startTime);
  void linearRampToValueAtTime(float value, double endTime);
  void exponentialRampToValueAtTime(float value, double endTime);

 private:
  float value_;
  float defaultValue_;
  float minValue_;
  float maxValue_;
  AudioContext *context_;
  ParamChange *currentChange_;
  std::set<ParamChange> changesQueue_;

  void checkValue(float value) const;
  double getStartTime();
  float getStartValue();
};

} // namespace audioapi
