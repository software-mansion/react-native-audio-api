#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>

namespace audioapi {

class AudioParam {
private:
    double value_;
    double defaultValue_;
    double minValue_;
    double maxValue_;

public:
    explicit AudioParam(double defaultValue, double minValue, double maxValue);
    double getValue() const;void setValue(double value);
  double getDefaultValue() const;
  double getMinValue() const;
  double getMaxValue() const;
  void setValueAtTime(double value, double startTime);
  void linearRampToValueAtTime(double value, double endTime);
  void exponentialRampToValueAtTime(double value, double endTime);
};

} // namespace audioapi
