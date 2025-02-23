#pragma once

#include <jsi/jsi.h>
#include <vector>
#include <memory>

namespace audioapi {
using namespace facebook;

class AudioSessionOptions {
 public:
  enum class IOSCategory {
    RECORD,
    AMBIENT,
    PLAYBACK,
    MULTI_ROUTE,
    SOLO_AMBIENT,
    PLAYBACK_AND_RECORD,
  };

  enum class IOSMode {
    DEFAULT,
    GAME_CHAT,
    VIDEO_CHAT,
    VOICE_CHAT,
    MEASUREMENT,
    VOICE_PROMPT,
    SPOKEN_AUDIO,
    MOVIE_PLAYBACK,
    VIDEO_RECORDING,
  };

  enum class IOSCategoryOption {
    DUCK_OTHERS,
    ALLOW_AIR_PLAY,
    MIX_WITH_OTHERS,
    ALLOW_BLUETOOTH,
    DEFAULT_TO_SPEAKER,
    ALLOW_BLUETOOTH_A2DP,
    OVERRIDE_MUTED_MICROPHONE_INTERRUPTION,
    INTERRUPT_SPOKEN_AUDIO_AND_MIX_WITH_OTHERS,
  };

  enum class InterruptionMode {
    MANUAL,
    DISABLED,
    AUTOMATIC,
  };

  IOSMode iosMode;
  IOSCategory iosCategory;
  bool androidForegroundService;
  InterruptionMode interruptionMode;
  std::vector<IOSCategoryOption> iosCategoryOptions;

  static std::shared_ptr<AudioSessionOptions> createFromJSIObject(const jsi::Value &jsiValue, jsi::Runtime &runtime) {
    auto jsiOptions = jsiValue.getObject(runtime);
    auto options = std::make_shared<AudioSessionOptions>();

    options->iosMode = static_cast<IOSMode>(jsiOptions.getProperty(runtime, "iosMode").getNumber());
    options->iosCategory = static_cast<IOSCategory>(jsiOptions.getProperty(runtime, "iosCategory").getNumber());
    options->androidForegroundService = jsiOptions.getProperty(runtime, "androidForegroundService").getBool();
    options->interruptionMode = static_cast<InterruptionMode>(jsiOptions.getProperty(runtime, "interruptionMode").getNumber());

    auto iosCategoryOptionsArray = jsiOptions.getProperty(runtime, "iosCategoryOptions").getObject(runtime).getArray(runtime);
    size_t iosCategoryOptionsSize = iosCategoryOptionsArray.size(runtime);

    for (size_t i = 0; i < iosCategoryOptionsSize; i++) {
      auto optionElement = iosCategoryOptionsArray.getValueAtIndex(runtime, i);

      options->iosCategoryOptions.push_back(static_cast<IOSCategoryOption>(optionElement.getNumber()));
    }

    return options;
  }

  jsi::Value toJSIObject(jsi::Runtime &runtime) const {
    auto jsiOptions = jsi::Object(runtime);

    jsiOptions.setProperty(runtime, "iosMode", jsi::Value(static_cast<double>(iosMode)));
    jsiOptions.setProperty(runtime, "iosCategory", jsi::Value(static_cast<double>(iosCategory)));
    jsiOptions.setProperty(runtime, "androidForegroundService", jsi::Value(androidForegroundService));
    jsiOptions.setProperty(runtime, "interruptionMode", jsi::Value(static_cast<double>(interruptionMode)));

    auto iosCategoryOptionsArray = jsi::Array(runtime, iosCategoryOptions.size());

    for (size_t i = 0; i < iosCategoryOptions.size(); i++) {
      iosCategoryOptionsArray.setValueAtIndex(runtime, i, jsi::Value(static_cast<double>(iosCategoryOptions[i])));
    }

    jsiOptions.setProperty(runtime, "iosCategoryOptions", iosCategoryOptionsArray);

    return jsiOptions;
  }
};

} // namespace audioapi
