#include <audioapi/utils/AudioRecorderOptions.h>

#include <jsi/jsi.h>

namespace audioapi {

AudioRecorderOptions AudioRecorderOptions::CreateFromJSIValue(
    facebook::jsi::Runtime &runtime,
    const facebook::jsi::Value &value) {
  AudioRecorderOptions options;

  if (!value.isObject()) {
    return options;
  }

  auto jsOptions = value.getObject(runtime);

  auto androidInputPreset = jsOptions.getProperty(runtime, "androidInputPreset");
  if (androidInputPreset.isString()) {
    options.androidInputPreset = androidInputPreset.getString(runtime).utf8(runtime);
  }

  auto iosVoiceProcessing = jsOptions.getProperty(runtime, "iosVoiceProcessing");
  if (iosVoiceProcessing.isBool()) {
    options.iosVoiceProcessing = iosVoiceProcessing.getBool();
  }

  return options;
}

} // namespace audioapi
