#pragma once

#include <cstdint>
#include <string_view>
namespace audioapi {

enum class AutomationEventType : uint8_t {
  LINEAR_RAMP,
  EXPONENTIAL_RAMP,
  SET_VALUE,
  SET_TARGET,
  SET_VALUE_CURVE,
};

inline std::string_view toString(AutomationEventType type) {
  switch (type) {
    case AutomationEventType::LINEAR_RAMP:
      return "LinearRampToValueAtTime";
    case AutomationEventType::EXPONENTIAL_RAMP:
      return "ExponentialRampToValueAtTime";
    case AutomationEventType::SET_VALUE:
      return "SetValueAtTime";
    case AutomationEventType::SET_TARGET:
      return "SetTargetAtTime";
    case AutomationEventType::SET_VALUE_CURVE:
      return "SetValueCurveAtTime";
  }
  return "Unknown";
}

} // namespace audioapi
