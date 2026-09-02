#pragma once

#include <audioapi/core/types/PannerTypes.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace audioapi::panner {

/// Spatialization math uses double to match Web Audio / WPT reference formulas
/// (JS Number). Sample buffers stay float32; callers cast at the last step.
constexpr double DEG_90 = 90.0;
constexpr double DEG_180 = 180.0;
constexpr double DEG_270 = 270.0;
constexpr double DEG_360 = 360.0;
constexpr double DEG_450 = 450.0;
constexpr double PI = std::numbers::pi_v<double>;

struct Vec3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

inline double dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline double magnitude(const Vec3 &v) {
  return std::sqrt(dot(v, v));
}

inline Vec3 subtract(const Vec3 &a, const Vec3 &b) {
  return {.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}

inline Vec3 scale(const Vec3 &v, double s) {
  return {.x = v.x * s, .y = v.y * s, .z = v.z * s};
}

inline Vec3 normalize(const Vec3 &v) {
  const double mag = magnitude(v);
  if (mag == 0.0) {
    return {};
  }
  return scale(v, 1.0 / mag);
}

inline Vec3 cross(const Vec3 &a, const Vec3 &b) {
  return {
      .x = a.y * b.z - a.z * b.y,
      .y = a.z * b.x - a.x * b.z,
      .z = a.x * b.y - a.y * b.x,
  };
}

/// https://www.w3.org/TR/webaudio-1.0/#azimuth-elevation
/// Elevation calculation is not implemented due to elevation not being used.
inline double computeAzimuth(
    const Vec3 &sourcePosition,
    const Vec3 &listenerPosition,
    const Vec3 &listenerForward,
    const Vec3 &listenerUp) {

  Vec3 sourceListener = subtract(sourcePosition, listenerPosition);
  sourceListener = normalize(sourceListener);
  if (magnitude(sourceListener) == 0.0) {
    return 0.0;
  }

  Vec3 listenerRight = cross(listenerForward, listenerUp);
  if (magnitude(listenerRight) == 0.0) {
    return 0.0;
  }

  const Vec3 listenerRightNorm = normalize(listenerRight);
  const Vec3 listenerForwardNorm = normalize(listenerForward);
  const Vec3 up = cross(listenerRightNorm, listenerForwardNorm);

  const double upProjection = dot(sourceListener, up);
  Vec3 projectedSource = subtract(sourceListener, scale(up, upProjection));
  if (magnitude(projectedSource) == 0.0) {
    return 0.0;
  }
  projectedSource = normalize(projectedSource);

  double azimuth =
      DEG_180 * std::acos(std::clamp(dot(projectedSource, listenerRightNorm), -1.0, 1.0)) / PI;

  const double frontBack = dot(projectedSource, listenerForwardNorm);
  if (frontBack < 0.0) {
    azimuth = DEG_360 - azimuth;
  }

  if (azimuth >= 0.0 && azimuth <= DEG_270) {
    azimuth = DEG_90 - azimuth;
  } else {
    azimuth = DEG_450 - azimuth;
  }

  return azimuth;
}

inline double clampAzimuth(double azimuth) {
  azimuth = std::max(-DEG_180, azimuth);
  azimuth = std::min(DEG_180, azimuth);

  return azimuth;
}

inline double wrapAzimuth(double azimuth) {
  if (azimuth < -DEG_90) {
    azimuth = -DEG_180 - azimuth;
  } else if (azimuth > DEG_90) {
    azimuth = DEG_180 - azimuth;
  }
  return azimuth;
}

inline double computeDistance(const Vec3 &sourcePosition, const Vec3 &listenerPosition) {
  return magnitude(subtract(sourcePosition, listenerPosition));
}

inline double computeDistanceGain(
    DistanceModelType model,
    double distance,
    double refDistance,
    double maxDistance,
    double rolloffFactor) {
  switch (model) {
    case DistanceModelType::Linear: {
      const double dRefClamped = std::min(refDistance, maxDistance);
      const double dMaxClamped = std::max(refDistance, maxDistance);

      distance = std::clamp(distance, dRefClamped, dMaxClamped);

      const double f = std::clamp(rolloffFactor, 0.0, 1.0);

      if (dRefClamped == dMaxClamped) {
        return 1.0 - f;
      }
      return 1.0 - f * (distance - dRefClamped) / (dMaxClamped - dRefClamped);
    }
    case DistanceModelType::Inverse: {
      if (refDistance == 0.0) {
        return 0.0;
      }
      const double f = std::max(rolloffFactor, 0.0);
      distance = std::max(distance, refDistance);
      return refDistance / (refDistance + f * (distance - refDistance));
    }
    case DistanceModelType::Exponential: {
      if (refDistance == 0.0) {
        return 0.0;
      }
      const double f = std::max(rolloffFactor, 0.0);
      distance = std::max(distance, refDistance);
      return std::pow(distance / refDistance, -f);
    }
  }
  return 1.0;
}

inline double computeConeGain(
    const Vec3 &sourcePosition,
    const Vec3 &listenerPosition,
    const Vec3 &sourceOrientation,
    double coneInnerAngle,
    double coneOuterAngle,
    double coneOuterGain) {
  if (magnitude(sourceOrientation) == 0.0) {
    return 1.0;
  }
  if (coneInnerAngle == DEG_360 && coneOuterAngle == DEG_360) {
    return 1.0;
  }

  const Vec3 sourceToListener = normalize(subtract(listenerPosition, sourcePosition));
  const Vec3 normalizedOrientation = normalize(sourceOrientation);

  const double angle =
      DEG_180 * std::acos(std::clamp(dot(sourceToListener, normalizedOrientation), -1.0, 1.0)) / PI;
  const double absAngle = std::abs(angle);
  const double absInnerAngle = std::abs(coneInnerAngle) / 2.0;
  const double absOuterAngle = std::abs(coneOuterAngle) / 2.0;

  if (absAngle <= absInnerAngle) {
    return 1.0;
  }
  if (absAngle >= absOuterAngle) {
    return coneOuterGain;
  }

  const double x = (absAngle - absInnerAngle) / (absOuterAngle - absInnerAngle);
  return (1.0 - x) + coneOuterGain * x;
}

} // namespace audioapi::panner
