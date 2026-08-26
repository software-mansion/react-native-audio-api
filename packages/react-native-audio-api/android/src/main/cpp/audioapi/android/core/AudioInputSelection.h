#pragma once

#include <oboe/Oboe.h>
#include <cstdint>

namespace audioapi {

/// @brief Process-wide choice of the capture device every AndroidAudioRecorder
/// opens its input stream on, set from Kotlin by AudioManager.setInputDevice.
///
/// Android routes capture per process rather than per stream, and the selection
/// arrives through the AudioAPIModule TurboModule, which knows nothing about the
/// individual recorders. It therefore cannot live on a recorder and is kept here
/// instead. Nothing in the shared common/cpp layer depends on it.
///
/// A stream reads the selection once, while it opens, and stays bound to that
/// device until it is reopened. The running-capture count exists so that a
/// selection made while a stream is running can be refused instead of being
/// silently deferred to the next start().
namespace AudioInputSelection {

/// @brief Leaves the capture device to the platform, which is Oboe's default.
constexpr int32_t kSystemDefaultDeviceId = oboe::kUnspecified;

/// @param deviceId An Android AudioDeviceInfo id, or kSystemDefaultDeviceId to
/// hand the choice back to the platform.
/// @returns false when a capture stream is running and the requested device
/// differs from the current selection. The selection is then left unchanged,
/// because a running stream cannot be moved onto it.
bool setPreferredDeviceId(int32_t deviceId);

int32_t getPreferredDeviceId();

/// @brief Reports that a recorder holds the selection: it is about to read it,
/// or is already feeding audio from it. Must be paired with captureStopped(),
/// including on teardown, and calls must balance.
void captureStarted();
void captureStopped();

} // namespace AudioInputSelection
} // namespace audioapi
