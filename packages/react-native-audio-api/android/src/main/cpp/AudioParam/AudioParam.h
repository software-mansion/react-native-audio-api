#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <optional>

#include "ParamChange.h"

namespace audioapi {

class AudioParam {
 public:
  explicit AudioParam(float defaultValue, float minValue, float maxValue);
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
  std::priority_queue<ParamChange, std::vector<ParamChange>> changesQueue_;
  ParamChange *currentChange_;

  float checkValue(float value) const;
  double getStartTime();
  float getStartValue();

};

} // namespace audioapi
