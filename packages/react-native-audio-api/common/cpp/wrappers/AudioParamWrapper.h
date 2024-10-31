#pragma once

#include <memory>

#include "AudioParam.h"

namespace audioapi {

class AudioParamWrapper {
 public:
  explicit AudioParamWrapper(const std::shared_ptr<AudioParam> &param);

  [[nodiscard]] float getValue() const;
  void setValue(float value) const;
  [[nodiscard]] float getDefaultValue() const;
  [[nodiscard]] float getMinValue() const;
  [[nodiscard]] float getMaxValue() const;
  void setValueAtTime(float value, double startTime) const;
  void linearRampToValueAtTime(float value, double endTime) const;
  void exponentialRampToValueAtTime(float value, double endTime) const;

 private:
  std::shared_ptr<AudioParam> param_;
};
} // namespace audioapi
