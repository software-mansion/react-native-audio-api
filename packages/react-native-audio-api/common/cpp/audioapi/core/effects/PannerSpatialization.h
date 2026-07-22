#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>

#include <algorithm>
#include <cmath>

namespace audioapi::panner {

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
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3 scale(const Vec3 &v, float s) {
  return {v.x * s, v.y * s, v.z * s};
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
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x,
  };
}

/// Azimuth in degrees — https://webaudio.github.io/web-audio-api/#Spatialization-azimuth-elevation
inline float computeAzimuth(
    const Vec3 &sourcePosition,
    const Vec3 &listenerPosition,
    const Vec3 &listenerForward,
    const Vec3 &listenerUp) {
  Vec3 sourceListener = subtract(sourcePosition, listenerPosition);
  if (magnitude(sourceListener) == 0.0f) {
    return 0.0f;
  }
  sourceListener = normalize(sourceListener);

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
      180.0f * std::acos(std::clamp(dot(projectedSource, listenerRightNorm), -1.0f, 1.0f)) / PI;

  if (dot(projectedSource, listenerForwardNorm) < 0.0f) {
    azimuth = 360.0f - azimuth;
  }

  if (azimuth >= 0.0f && azimuth <= 270.0f) {
    azimuth = 90.0f - azimuth;
  } else {
    azimuth = 450.0f - azimuth;
  }

  return azimuth;
}

inline float wrapAzimuthForEqualPower(float azimuth) {
  azimuth = std::max(-180.0f, azimuth);
  azimuth = std::min(180.0f, azimuth);
  if (azimuth < -90.0f) {
    azimuth = -180.0f - azimuth;
  } else if (azimuth > 90.0f) {
    azimuth = 180.0f - azimuth;
  }
  return azimuth;
}

/// Returns azimuth wrapped to [-90, 90] and writes equal-power L/R gains.
inline float computeEqualPowerGains(float azimuth, bool monoInput, float &gainL, float &gainR) {
  azimuth = wrapAzimuthForEqualPower(azimuth);

  float x = 0.0f;
  if (monoInput) {
    x = (azimuth + 90.0f) / 180.0f;
  } else if (azimuth <= 0.0f) {
    x = (azimuth + 90.0f) / 90.0f;
  } else {
    x = azimuth / 90.0f;
  }

  const float angle = x * (PI / 2.0f);
  gainL = std::cos(angle);
  gainR = std::sin(angle);
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
  const float dRef = static_cast<float>(refDistance);
  const float dMax = static_cast<float>(maxDistance);
  float f = static_cast<float>(rolloffFactor);

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
  if (coneInnerAngle == 360.0 && coneOuterAngle == 360.0) {
    return 1.0f;
  }

  // Vector from the source toward the listener (matches browser ConeEffect).
  const Vec3 sourceToListener = normalize(subtract(listenerPosition, sourcePosition));
  const Vec3 normalizedOrientation = normalize(sourceOrientation);

  const float angle = 180.0f *
      std::acos(std::clamp(dot(sourceToListener, normalizedOrientation), -1.0f, 1.0f)) / PI;
  const float absAngle = std::abs(angle);
  const float absInnerAngle = static_cast<float>(std::abs(coneInnerAngle) / 2.0);
  const float absOuterAngle = static_cast<float>(std::abs(coneOuterAngle) / 2.0);

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
