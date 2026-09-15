#pragma once

#include <oboe/Oboe.h>
#include <cstdint>

namespace audioapi::AudioInputSelection {

/// @brief Process-wide choice of the capture device every AndroidAudioRecorder
/// opens its input stream on, set by the Kotlin AudioAPIModule.setInputDevice.
///
/// The selection arrives through the TurboModule, which has no reference to the
/// individual recorders, so it is kept here rather than on a recorder.
///
/// A stream reads the selection once, while it opens, and stays bound to that
/// device until it is reopened. The running-capture count lets a selection made
/// while a stream is running be refused instead of silently deferred to the
/// next start().

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
/// or is already feeding audio from it. Every call must be balanced by
/// captureStopped(), including on teardown.
void captureStarted();
void captureStopped();

} // namespace audioapi::AudioInputSelection
