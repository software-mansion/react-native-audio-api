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

public:
  explicit AudioParamWrapper(const std::shared_ptr<AudioParam> &param);

private:
    std::shared_ptr<AudioParam> param_;
#else

public:
  explicit AudioParamWrapper(const std::shared_ptr<IOSAudioParam> &param);

private:
  std::shared_ptr<IOSAudioParam> param_;
#endif

public:
  float getValue() const;
  void setValue(float value) const;
  float getDefaultValue() const;
  float getMinValue() const;
  float getMaxValue() const;
  void setValueAtTime(float value, double startTime) const;
  void linearRampToValueAtTime(float value, double endTime) const;
  void exponentialRampToValueAtTime(float value, double endTime) const;
};
} // namespace audioapi
