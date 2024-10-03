#pragma once

#include <functional>
#include <memory>

namespace audioapi {

class ParamChange {
public:
    ParamChange(double startTime, double endTime, float startValue, float endValue,
                std::function<float(double, double, float, float, double)> calculateValue);

    double getEndTime() const;
    double getStartTime() const;
    bool operator<(const ParamChange& other) const;
    float getValueAtTime(double time) const;

private:
    double startTime_;
    double endTime_;
    float startValue_;
    float endValue_;
    std::function<float(double, double, float, float, double)> calculateValue_;

};

} // namespace audioapi

