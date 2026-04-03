#pragma once

#include <memory>
#include <utility>
#include "audioapi/core/utils/automation/RenderAutomationEvent.hpp"
#include "audioapi/dsp/AudioUtils.hpp"
#include "audioapi/utils/AudioArray.hpp"

namespace audioapi {

class AutomationRenderEventFactory {
 public:
  static RenderAutomationEvent createSetValueEvent(float value, double startTime) {
    auto calculateValue =
        [](double startTime, double /* endTime */, float startValue, float endValue, double time) {
          if (time < startTime) {
            return startValue;
          }

          return endValue;
        };

    return RenderAutomationEvent(
        startTime,
        startTime,
        value,
        value,
        std::move(calculateValue),
        AutomationEventType::SET_VALUE);
  }

  static RenderAutomationEvent createLinearRampEvent(float value, double endTime) {
    auto calculateValue =
        [](double startTime, double endTime, float startValue, float endValue, double time) {
          if (time < startTime) {
            return startValue;
          }

          if (time < endTime) {
            return static_cast<float>(
                startValue + (endValue - startValue) * (time - startTime) / (endTime - startTime));
          }

          return endValue;
        };

    return RenderAutomationEvent(
        0.0, endTime, 0.0f, value, std::move(calculateValue), AutomationEventType::LINEAR_RAMP);
  }

  static RenderAutomationEvent createExponentialRampEvent(float value, double endTime) {
    auto calculateValue =
        [](double startTime, double endTime, float startValue, float endValue, double time) {
          if (startValue * endValue < 0 || startValue == 0) {
            return startValue;
          }

          if (time < startTime) {
            return startValue;
          }

          if (time < endTime) {
            return static_cast<float>(
                startValue *
                pow(endValue / startValue, (time - startTime) / (endTime - startTime)));
          }

          return endValue;
        };

    return RenderAutomationEvent(
        0.0,
        endTime,
        0.0f,
        value,
        std::move(calculateValue),
        AutomationEventType::EXPONENTIAL_RAMP);
  }

  static RenderAutomationEvent
  createSetTargetEvent(float target, double startTime, double timeConstant) {
    auto calculateValue = [timeConstant, target](
                              double startTime,
                              double /* endTime */,
                              float startValue,
                              float /* endValue */,
                              double time) {
      if (timeConstant == 0) {
        return target;
      }

      if (time < startTime) {
        return startValue;
      }

      return static_cast<float>(
          target + (startValue - target) * exp(-(time - startTime) / timeConstant));
    };

    return RenderAutomationEvent(
        startTime,
        startTime, // SetTarget events have infinite duration conceptually
        0.0f,
        0.0f, // End value is not meaningful for infinite events
        std::move(calculateValue),
        AutomationEventType::SET_TARGET);
  }

  static RenderAutomationEvent createSetValueCurveEvent(
      const std::shared_ptr<AudioArray> &values,
      size_t length,
      double startTime,
      double duration) {
    auto calculateValue =
        [values, length](
            double startTime, double endTime, float startValue, float endValue, double time) {
          if (time < startTime) {
            return startValue;
          }

          if (time < endTime) {
            // Calculate position in the array based on time progress
            auto k = static_cast<int>(std::floor(
                static_cast<double>(length - 1) / (endTime - startTime) * (time - startTime)));
            // Calculate interpolation factor between adjacent array elements
            auto factor = static_cast<float>(
                (time - startTime) * static_cast<double>(length - 1) / (endTime - startTime) - k);
            return dsp::linearInterpolate(values->span(), k, k + 1, factor);
          }

          return endValue;
        };

    return RenderAutomationEvent(
        startTime,
        startTime + duration,
        0.0f,
        values->span()[length - 1],
        std::move(calculateValue),
        AutomationEventType::SET_VALUE_CURVE);
  }
};

} // namespace audioapi