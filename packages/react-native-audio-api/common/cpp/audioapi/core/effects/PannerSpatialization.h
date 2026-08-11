#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>

#include <algorithm>
#include <cmath>

namespace audioapi::panner {

constexpr float DEG_90 = 90.0f;
constexpr float DEG_180 = 180.0f;
constexpr float DEG_270 = 270.0f;
constexpr float DEG_360 = 360.0f;
constexpr float DEG_450 = 450.0f;

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

inline float dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float magnitude(const Vec3 &v) {
  return std::sqrt(dot(v, v));
}

inline Vec3 subtract(const Vec3 &a, const Vec3 &b) {
  return {.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}

inline Vec3 scale(const Vec3 &v, float s) {
  return {.x = v.x * s, .y = v.y * s, .z = v.z * s};
}

inline Vec3 normalize(const Vec3 &v) {
  const float mag = magnitude(v);
  if (mag == 0.0f) {
    return {};
  }
  return scale(v, 1.0f / mag);
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
inline float computeAzimuth(
    const Vec3 &sourcePosition,
    const Vec3 &listenerPosition,
    const Vec3 &listenerForward,
    const Vec3 &listenerUp) {

  Vec3 sourceListener = subtract(sourcePosition, listenerPosition);
  sourceListener = normalize(sourceListener);
  if (magnitude(sourceListener) == 0.0f) {
    return 0.0f;
  }

  Vec3 listenerRight = cross(listenerForward, listenerUp);
  if (magnitude(listenerRight) == 0.0f) {
    return 0.0f;
  }

  const Vec3 listenerRightNorm = normalize(listenerRight);
  const Vec3 listenerForwardNorm = normalize(listenerForward);
  const Vec3 up = cross(listenerRightNorm, listenerForwardNorm);

  const float upProjection = dot(sourceListener, up);
  Vec3 projectedSource = normalize(subtract(sourceListener, scale(up, upProjection)));

  float azimuth =
      DEG_180 * std::acos(std::clamp(dot(projectedSource, listenerRightNorm), -1.0f, 1.0f)) / PI;

  const float frontBack = dot(projectedSource, listenerForwardNorm);
  if (frontBack < 0.0f) {
    azimuth = DEG_360 - azimuth;
  }

  if (azimuth >= 0.0f && azimuth <= DEG_270) {
    azimuth = DEG_90 - azimuth;
  } else {
    azimuth = DEG_450 - azimuth;
  }

  return azimuth;
}

inline float clampAzimuth(float azimuth) {
  azimuth = std::max(-1 * DEG_180, azimuth);
  azimuth = std::min(DEG_180, azimuth);

  return azimuth;
}

inline float wrapAzimuth(float azimuth) {
  if (azimuth < -1 * DEG_90) {
    azimuth = -1 * DEG_180 - azimuth;
  } else if (azimuth > DEG_90) {
    azimuth = DEG_180 - azimuth;
  }
  return azimuth;
}

inline float computeDistance(const Vec3 &sourcePosition, const Vec3 &listenerPosition) {
  return magnitude(subtract(sourcePosition, listenerPosition));
}

inline float computeDistanceGain(
    DistanceModelType model,
    float distance,
    double refDistance,
    double maxDistance,
    double rolloffFactor) {
  const auto dRef = static_cast<float>(refDistance);
  const auto dMax = static_cast<float>(maxDistance);
  auto f = static_cast<float>(rolloffFactor);

  switch (model) {
    case DistanceModelType::Linear: {
      const float dRefClamped = std::min(dRef, dMax);
      const float dMaxClamped = std::max(dRef, dMax);

      distance = std::clamp(distance, dRefClamped, dMaxClamped);

      f = std::clamp(f, 0.0f, 1.0f);

      if (dRefClamped == dMaxClamped) {
        return 1.0f - f;
      }
      return 1.0f - f * (distance - dRefClamped) / (dMaxClamped - dRefClamped);
    }
    case DistanceModelType::Inverse: {
      if (dRef == 0.0f) {
        return 0.0f;
      }
      f = std::max(f, 0.0f);
      distance = std::max(distance, dRef);
      return dRef / (dRef + f * (distance - dRef));
    }
    case DistanceModelType::Exponential: {
      if (dRef == 0.0f) {
        return 0.0f;
      }
      f = std::max(f, 0.0f);
      distance = std::max(distance, dRef);
      return std::pow(distance / dRef, -f);
    }
  }
  return 1.0f;
}

inline float computeConeGain(
    const Vec3 &sourcePosition,
    const Vec3 &listenerPosition,
    const Vec3 &sourceOrientation,
    double coneInnerAngle,
    double coneOuterAngle,
    double coneOuterGain) {
  if (magnitude(sourceOrientation) == 0.0f) {
    return 1.0f;
  }
  if (coneInnerAngle == DEG_360 && coneOuterAngle == DEG_360) {
    return 1.0f;
  }

  const Vec3 sourceToListener = normalize(subtract(listenerPosition, sourcePosition));
  const Vec3 normalizedOrientation = normalize(sourceOrientation);

  const float angle = DEG_180 *
      std::acos(std::clamp(dot(sourceToListener, normalizedOrientation), -1.0f, 1.0f)) / PI;
  const float absAngle = std::abs(angle);
  const auto absInnerAngle = static_cast<float>(std::abs(coneInnerAngle) / 2.0);
  const auto absOuterAngle = static_cast<float>(std::abs(coneOuterAngle) / 2.0);

  if (absAngle <= absInnerAngle) {
    return 1.0f;
  }
  if (absAngle >= absOuterAngle) {
    return static_cast<float>(coneOuterGain);
  }

  const float x = (absAngle - absInnerAngle) / (absOuterAngle - absInnerAngle);
  return (1.0f - x) + static_cast<float>(coneOuterGain) * x;
}

} // namespace audioapi::panner
