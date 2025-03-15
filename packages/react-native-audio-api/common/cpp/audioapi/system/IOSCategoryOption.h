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

  static Option fromString(const std::string &optionName) {
    if (optionName == "duckOthers") {
      return Option::DUCK_OTHERS;
    }

    if (optionName == "allowAirPlay") {
      return Option::ALLOW_AIR_PLAY;
    }

    if (optionName == "mixWithOthers") {
      return Option::MIX_WITH_OTHERS;
    }

    if (optionName == "allowBluetooth") {
      return Option::ALLOW_BLUETOOTH;
    }

    if (optionName == "defaultToSpeaker") {
      return Option::DEFAULT_TO_SPEAKER;
    }

    if (optionName == "allowBluetoothA2DP") {
      return Option::ALLOW_BLUETOOTH_A2DP;
    }

    if (optionName == "overrideMutedMicrophoneInterruption") {
      return Option::OVERRIDE_MUTED_MICROPHONE_INTERRUPTION;
    }

    if (optionName == "interruptSpokenAudioAndMixWithOthers") {
      return Option::INTERRUPT_SPOKEN_AUDIO_AND_MIX_WITH_OTHERS;
    }

    return Option::MIX_WITH_OTHERS;
  }

  static IOSCategoryOption fromJSIValue(const jsi::Value &value, jsi::Runtime &runtime) {
    std::string optionName = value.asString(runtime).utf8(runtime);

    return IOSCategoryOption(IOSCategoryOption::fromString(optionName));
  }

  Option value() const {
    return option_;
  }

  std::string strValue() const {
    switch (option_) {
      case Option::DUCK_OTHERS:
        return "duckOthers";
      case Option::ALLOW_AIR_PLAY:
        return "allowAirPlay";
      case Option::MIX_WITH_OTHERS:
        return "mixWithOthers";
      case Option::ALLOW_BLUETOOTH:
        return "allowBluetooth";
      case Option::DEFAULT_TO_SPEAKER:
        return "defaultToSpeaker";
      case Option::ALLOW_BLUETOOTH_A2DP:
        return "allowBluetoothA2DP";
      case Option::OVERRIDE_MUTED_MICROPHONE_INTERRUPTION:
        return "overrideMutedMicrophoneInterruption";
      case Option::INTERRUPT_SPOKEN_AUDIO_AND_MIX_WITH_OTHERS:
        return "interruptSpokenAudioAndMixWithOthers";
    }
  }

  jsi::Value toJSIValue(jsi::Runtime &runtime) const {
    return jsi::String::createFromUtf8(runtime, strValue());
  }

 private:
  Option option_;
};

} // namespace audioapi
