#pragma once

#include <functional>
#include <memory>

namespace audioapi {

class ParamChange {
 public:
  explicit ParamChange(
      double startTime,
      double endTime,
      float startValue,
      float endValue,
      std::function<float(double, double, float, float, double)>
          calculateValue);

  double getEndTime() const;
  double getStartTime() const;
  float getEndValue() const;
  float getValueAtTime(double time) const;
  bool operator<(const ParamChange &other) const;

 private:
  double startTime_;
  double endTime_;
  float startValue_;
  float endValue_;
  std::function<float(double, double, float, float, double)> calculateValue_;
};

} // namespace audioapi
