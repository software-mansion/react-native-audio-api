#pragma once

#include <memory>

#ifdef ANDROID
#include "AudioParam.h"
#else
#include "IOSAudioParam.h"
#endif

namespace audioapi {

class AudioParamWrapper {
#ifdef ANDROID

 protected:
  AudioParam *param_;

 public:
  explicit AudioParamWrapper(AudioParam *param);
#else

 protected:
  std::shared_ptr<IOSAudioParam> param_;

 public:
  explicit AudioParamWrapper(std::shared_ptr<IOSAudioParam> param);
#endif

 private:
  double defaultValue_;
  double minValue_;
  double maxValue_;

 public:
  double getValue();
  void setValue(double value);
  double getDefaultValue() const;
  double getMinValue() const;
  double getMaxValue() const;
  void setValueAtTime(double value, double startTime);
  void linearRampToValueAtTime(double value, double endTime);
  void exponentialRampToValueAtTime(double value, double endTime);
};
} // namespace audioapi
