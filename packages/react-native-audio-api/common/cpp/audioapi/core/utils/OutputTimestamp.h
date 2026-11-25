#pragma once

#include <audioapi/core/types/AudioTimestamp.h>

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace audioapi {

/// The JS runtime's `performance.now()` reads this clock on Android. On iOS it does not, see
/// `IOSAudioPlayer::presentationClockNowNs`.
[[nodiscard]] inline std::int64_t steadyClockNowNs() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

/// A context sample frame and the moment it leaves the output device, on the clock the player's
/// `presentationClockNowNs()` reads.
struct OutputPresentation {
  /// Signed: a platform may report a device position from before the context's first frame.
  std::int64_t contextFrame = 0;
  std::int64_t presentationTimeNs = 0;
};

/// @brief Spec `getOutputTimestamp()`: the frame leaving the device now, and the time of now on
/// the clock @p presentation is expressed in.
/// @param presentation the latest frame/moment pair the platform reported.
/// @param renderedFrames frames the graph has rendered
/// @param nowNs the moment to report the position at, on that same clock.
[[nodiscard]] inline AudioTimestamp outputTimestampAt(
    const OutputPresentation &presentation,
    float sampleRate,
    std::size_t renderedFrames,
    bool isPlaying,
    std::int64_t nowNs) {
  constexpr double NANOSECONDS_PER_SECOND = 1e9;
  constexpr double NANOSECONDS_PER_MILLISECOND = 1e6;

  auto frame = static_cast<double>(presentation.contextFrame);
  auto timeNs = static_cast<double>(presentation.presentationTimeNs);

  // interpolation
  if (isPlaying) {
    frame += (static_cast<double>(nowNs) - timeNs) * sampleRate / NANOSECONDS_PER_SECOND;
    timeNs = static_cast<double>(nowNs);
  }

  const auto rendered = static_cast<double>(renderedFrames);
  // can happen if the audio stalls or is interrupted
  if (frame > rendered) {
    timeNs -= (frame - rendered) / sampleRate * NANOSECONDS_PER_SECOND;
    frame = rendered;
  }

  // frame 0 hasnt reach the speaker yet
  if (frame < 0.0) {
    timeNs -= frame / sampleRate * NANOSECONDS_PER_SECOND;
    frame = 0.0;
  }

  return {
      .contextTime = frame / sampleRate, .performanceTime = timeNs / NANOSECONDS_PER_MILLISECOND};
}

} // namespace audioapi
