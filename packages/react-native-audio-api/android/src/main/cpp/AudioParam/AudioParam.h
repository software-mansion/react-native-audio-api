#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>

namespace audioapi {

class AudioParam {

 public:
  explicit AudioParam(float defaultValue, float minValue, float maxValue);
  float getValue() const;
  void setValue(float value);
  float getDefaultValue() const;
  float getMinValue() const;
  float getMaxValue() const;
  void setValueAtTime(float value, float startTime);
  void linearRampToValueAtTime(float value, float endTime);
  void exponentialRampToValueAtTime(float value, float endTime);

private:
    float value_;
    float defaultValue_;
    float minValue_;
    float maxValue_;
};

} // namespace audioapi
