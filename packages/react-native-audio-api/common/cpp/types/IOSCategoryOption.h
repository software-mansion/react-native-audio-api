#pragma once

#include <jsi/jsi.h>
#include <string>

namespace audioapi {
using namespace facebook;

class IOSCategoryOption {
 public:
  enum Option {
    DUCK_OTHERS,
    ALLOW_AIR_PLAY,
    MIX_WITH_OTHERS,
    ALLOW_BLUETOOTH,
    DEFAULT_TO_SPEAKER,
    ALLOW_BLUETOOTH_A2DP,
    OVERRIDE_MUTED_MICROPHONE_INTERRUPTION,
    INTERRUPT_SPOKEN_AUDIO_AND_MIX_WITH_OTHERS,
  };

  IOSCategoryOption() = default;
  explicit constexpr IOSCategoryOption(Option option) : option_(option) {}

  constexpr operator Option() const { return option_; }
  explicit operator bool() const = delete;
  constexpr bool operator==(IOSCategoryOption other) const { return option_ == other.option_; }
  constexpr bool operator!=(IOSCategoryOption other) const { return option_ != other.option_; }

  static IOSCategoryOption fromJSIValue(const jsi::Value &value, jsi::Runtime &runtime) {
    std::string optionName = value.asString(runtime).utf8(runtime);

    if (optionName == "duckOthers") {
      return IOSCategoryOption(Option::DUCK_OTHERS);
    }

    if (optionName == "allowAirPlay") {
      return IOSCategoryOption(Option::ALLOW_AIR_PLAY);
    }

    if (optionName == "mixWithOthers") {
      return IOSCategoryOption(Option::MIX_WITH_OTHERS);
    }

    if (optionName == "allowBluetooth") {
      return IOSCategoryOption(Option::ALLOW_BLUETOOTH);
    }

    if (optionName == "defaultToSpeaker") {
      return IOSCategoryOption(Option::DEFAULT_TO_SPEAKER);
    }

    if (optionName == "allowBluetoothA2DP") {
      return IOSCategoryOption(Option::ALLOW_BLUETOOTH_A2DP);
    }

    if (optionName == "overrideMutedMicrophoneInterruption") {
      return IOSCategoryOption(Option::OVERRIDE_MUTED_MICROPHONE_INTERRUPTION);
    }

    if (optionName == "interruptSpokenAudioAndMixWithOthers") {
      return IOSCategoryOption(Option::INTERRUPT_SPOKEN_AUDIO_AND_MIX_WITH_OTHERS);
    }

    return IOSCategoryOption(Option::MIX_WITH_OTHERS);
  }

  jsi::Value toJSIValue(jsi::Runtime &runtime) const {
    switch (option_) {
      case Option::DUCK_OTHERS:
        return jsi::String::createFromUtf8(runtime, "duckOthers");
      case Option::ALLOW_AIR_PLAY:
        return jsi::String::createFromUtf8(runtime, "allowAirPlay");
      case Option::MIX_WITH_OTHERS:
        return jsi::String::createFromUtf8(runtime, "mixWithOthers");
      case Option::ALLOW_BLUETOOTH:
        return jsi::String::createFromUtf8(runtime, "allowBluetooth");
      case Option::DEFAULT_TO_SPEAKER:
        return jsi::String::createFromUtf8(runtime, "defaultToSpeaker");
      case Option::ALLOW_BLUETOOTH_A2DP:
        return jsi::String::createFromUtf8(runtime, "allowBluetoothA2DP");
      case Option::OVERRIDE_MUTED_MICROPHONE_INTERRUPTION:
        return jsi::String::createFromUtf8(runtime, "overrideMutedMicrophoneInterruption");
      case Option::INTERRUPT_SPOKEN_AUDIO_AND_MIX_WITH_OTHERS:
        return jsi::String::createFromUtf8(runtime, "interruptSpokenAudioAndMixWithOthers");
    }
  }

 private:
  Option option_;
};

} // namespace audioapi
