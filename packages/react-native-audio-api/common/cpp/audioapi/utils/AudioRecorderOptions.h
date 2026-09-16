#pragma once

#include <string>

namespace facebook {
namespace jsi {
class Runtime;
class Value;
} // namespace jsi
} // namespace facebook

namespace audioapi {

/// Creation-time configuration of the platform capture chain, mirroring the
/// JS-side `AudioRecorderOptions`. Every field is honoured by a single platform
/// and ignored by the other one.
struct AudioRecorderOptions {
  /// Name of the Oboe input preset, e.g. "voiceCommunication". An empty or
  /// unknown name leaves Oboe's own default in place. Android only.
  std::string androidInputPreset;

  /// Runs the capture chain through Apple's voice-processing I/O unit: echo
  /// cancellation, noise suppression and automatic gain control. iOS only.
  bool iosVoiceProcessing = false;

  /// Reads the options out of the object passed to `createAudioRecorder`.
  /// Missing or mistyped properties keep their default, so an absent options
  /// object yields the pre-existing platform behavior.
  static AudioRecorderOptions CreateFromJSIValue(
      facebook::jsi::Runtime &runtime,
      const facebook::jsi::Value &value);
};

} // namespace audioapi
