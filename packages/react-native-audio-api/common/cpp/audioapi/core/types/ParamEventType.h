#pragma once

#include <cstdint>
#include <string_view>
namespace audioapi {

enum class ParamEventType : uint8_t {
  LINEAR_RAMP,
  EXPONENTIAL_RAMP,
  SET_VALUE,
  SET_TARGET,
  SET_VALUE_CURVE,
};

inline std::string_view toString(ParamEventType type) {
  switch (type) {
    case ParamEventType::LINEAR_RAMP:
      return "LinearRampToValueAtTime";
    case ParamEventType::EXPONENTIAL_RAMP:
      return "ExponentialRampToValueAtTime";
    case ParamEventType::SET_VALUE:
      return "SetValueAtTime";
    case ParamEventType::SET_TARGET:
      return "SetTargetAtTime";
    case ParamEventType::SET_VALUE_CURVE:
      return "SetValueCurveAtTime";
  }
  return "Unknown";
}

} // namespace audioapi
