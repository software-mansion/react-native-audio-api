#include <audioapi/utils/AudioRecorderOptions.h>

#include <jsi/jsi.h>

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace audioapi {

namespace {

using AndroidInputPreset = AudioRecorderOptions::AndroidInputPreset;

constexpr std::array<std::pair<std::string_view, AndroidInputPreset>, 6> ANDROID_INPUT_PRESETS{{
    {"generic", AndroidInputPreset::Generic},
    {"camcorder", AndroidInputPreset::Camcorder},
    {"voiceRecognition", AndroidInputPreset::VoiceRecognition},
    {"voiceCommunication", AndroidInputPreset::VoiceCommunication},
    {"unprocessed", AndroidInputPreset::Unprocessed},
    {"voicePerformance", AndroidInputPreset::VoicePerformance},
}};

bool isAbsent(const facebook::jsi::Value &value) {
  return value.isUndefined() || value.isNull();
}

} // namespace

Result<AndroidInputPreset, std::string> AudioRecorderOptions::parseAndroidInputPreset(
    const std::string &name) {
  for (const auto &[presetName, preset] : ANDROID_INPUT_PRESETS) {
    if (presetName == name) {
      return Ok(preset);
    }
  }
  return Err("androidInputPreset '" + name + "' is not an AndroidInputPreset.");
}

Result<AudioRecorderOptions, std::string> AudioRecorderOptions::CreateFromJSIValue(
    facebook::jsi::Runtime &runtime,
    const facebook::jsi::Value &value) {
  AudioRecorderOptions options;

  if (isAbsent(value)) {
    return Ok(options);
  }
  if (!value.isObject()) {
    return Err(std::string("createAudioRecorder options must be an object."));
  }

  auto jsOptions = value.getObject(runtime);

  auto androidInputPreset = jsOptions.getProperty(runtime, "androidInputPreset");
  if (!isAbsent(androidInputPreset)) {
    if (!androidInputPreset.isString()) {
      return Err(std::string("androidInputPreset must be a string."));
    }
    auto presetResult =
        parseAndroidInputPreset(androidInputPreset.getString(runtime).utf8(runtime));
    if (presetResult.is_err()) {
      return Err(presetResult.unwrap_err());
    }
    options.androidInputPreset = presetResult.unwrap();
  }

  auto iosVoiceProcessing = jsOptions.getProperty(runtime, "iosVoiceProcessing");
  if (!isAbsent(iosVoiceProcessing)) {
    if (!iosVoiceProcessing.isBool()) {
      return Err(std::string("iosVoiceProcessing must be a boolean."));
    }
    options.iosVoiceProcessing = iosVoiceProcessing.getBool();
  }

  return Ok(options);
}

} // namespace audioapi
