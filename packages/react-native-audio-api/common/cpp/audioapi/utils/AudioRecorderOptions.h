#pragma once

#include <audioapi/utils/Result.hpp>

#include <cstdint>
#include <string>

namespace facebook::jsi {
class Runtime;
class Value;
} // namespace facebook::jsi

namespace audioapi {

/// Creation-time configuration of the platform capture chain, mirroring the
/// JS-side `AudioRecorderOptions`. Every field is honoured by a single platform
/// and ignored by the other one.
struct AudioRecorderOptions {
  /// Mirrors the JS `AndroidInputPreset` names; Android maps each to an Oboe input preset.
  enum class AndroidInputPreset : std::uint8_t {
    /// No preset is set, which leaves Oboe's own default (voiceRecognition) in place.
    PlatformDefault,
    Generic,
    Camcorder,
    VoiceRecognition,
    VoiceCommunication,
    Unprocessed,
    VoicePerformance,
  };

  /// Android only.
  AndroidInputPreset androidInputPreset = AndroidInputPreset::PlatformDefault;

  /// Runs the capture chain through Apple's voice-processing I/O unit: echo
  /// cancellation, noise suppression and automatic gain control. iOS only.
  bool iosVoiceProcessing = false;

  /// Reads the options out of the object passed to `createAudioRecorder`. An absent options
  /// object or property keeps its default; a property of the wrong type, or a preset name
  /// that is not an AndroidInputPreset, is an error rather than being silently ignored.
  static Result<AudioRecorderOptions, std::string> CreateFromJSIValue(
      facebook::jsi::Runtime &runtime,
      const facebook::jsi::Value &value);

  /// @param name One of the JS `AndroidInputPreset` names, e.g. "voiceCommunication".
  static Result<AndroidInputPreset, std::string> parseAndroidInputPreset(const std::string &name);
};

} // namespace audioapi
