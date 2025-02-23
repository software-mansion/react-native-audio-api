#pragma once

#include <jsi/jsi.h>
#include <string>

namespace audioapi {
using namespace facebook;

class AudioEventName {
 public:
  enum Name {
    PLAY,
    PAUSE,
    STOP,

    NEXT_TRACK,
    PREVIOUS_TRACK,

    CHANGE_POSITION,

    SEEK,
    SEEK_FORWARD,
    SEEK_BACKWARD,

    SKIP_FORWARD,
    SKIP_BACKWARD,

    ANDROID_CLOSE,
    ANDROID_SET_VOLUME,
    ANDROID_SET_RATING,

    IOS_TOGGLE_PLAY_PAUSE,
    IOS_ENABLE_LANGUAGE_OPTIONS,
    IOS_DISABLE_LANGUAGE_OPTIONS,
  };

  AudioEventName() = default;
  explicit constexpr AudioEventName(Name name) : name_(name) {}

  constexpr operator Name() const { return name_; }
  explicit operator bool() const = delete;
  constexpr bool operator==(AudioEventName other) const { return name_ == other.name_; }
  constexpr bool operator!=(AudioEventName other) const { return name_ != other.name_; }

  static AudioEventName fromJSIValue(const jsi::Value &value, jsi::Runtime &runtime) {
    std::string eventName = value.asString(runtime).utf8(runtime);

    if (eventName == "play") {
      return AudioEventName(Name::PLAY);
    }

    if (eventName == "pause") {
      return AudioEventName(Name::PAUSE);
    }

    if (eventName == "stop") {
      return AudioEventName(Name::STOP);
    }

    if (eventName == "nextTrack") {
      return AudioEventName(Name::NEXT_TRACK);
    }

    if (eventName == "previousTrack") {
      return AudioEventName(Name::PREVIOUS_TRACK);
    }

    if (eventName == "changePosition") {
      return AudioEventName(Name::CHANGE_POSITION);
    }

    if (eventName == "seek") {
      return AudioEventName(Name::SEEK);
    }

    if (eventName == "seekForward") {
      return AudioEventName(Name::SEEK_FORWARD);
    }

    if (eventName == "seekBackward") {
      return AudioEventName(Name::SEEK_BACKWARD);
    }

    if (eventName == "skipForward") {
      return AudioEventName(Name::SKIP_FORWARD);
    }

    if (eventName == "skipBackward") {
      return AudioEventName(Name::SKIP_BACKWARD);
    }

    if (eventName == "androidClose") {
      return AudioEventName(Name::ANDROID_CLOSE);
    }

    if (eventName == "androidSetVolume") {
      return AudioEventName(Name::ANDROID_SET_VOLUME);
    }

    if (eventName == "androidSetRating") {
      return AudioEventName(Name::ANDROID_SET_RATING);
    }

    if (eventName == "iosTogglePlayPause") {
      return AudioEventName(Name::IOS_TOGGLE_PLAY_PAUSE);
    }

    if (eventName == "iosEnableLanguageOptions") {
      return AudioEventName(Name::IOS_ENABLE_LANGUAGE_OPTIONS);
    }

    return AudioEventName(Name::IOS_DISABLE_LANGUAGE_OPTIONS);
  }

  jsi::Value toJSIValue(jsi::Runtime &runtime) {
    switch (name_) {
      case Name::PLAY:
        return jsi::String::createFromUtf8(runtime, "play");
      case Name::PAUSE:
        return jsi::String::createFromUtf8(runtime, "pause");
      case Name::STOP:
        return jsi::String::createFromUtf8(runtime, "stop");
      case Name::NEXT_TRACK:
        return jsi::String::createFromUtf8(runtime, "nextTrack");
      case Name::PREVIOUS_TRACK:
        return jsi::String::createFromUtf8(runtime, "previousTrack");
      case Name::CHANGE_POSITION:
        return jsi::String::createFromUtf8(runtime, "changePosition");
      case Name::SEEK:
        return jsi::String::createFromUtf8(runtime, "seek");
      case Name::SEEK_FORWARD:
        return jsi::String::createFromUtf8(runtime, "seekForward");
      case Name::SEEK_BACKWARD:
        return jsi::String::createFromUtf8(runtime, "seekBackward");
        case Name::SKIP_FORWARD:
        return jsi::String::createFromUtf8(runtime, "skipForward");
      case Name::SKIP_BACKWARD:
        return jsi::String::createFromUtf8(runtime, "skipBackward");
      case Name::ANDROID_CLOSE:
        return jsi::String::createFromUtf8(runtime, "androidClose");
      case Name::ANDROID_SET_VOLUME:
        return jsi::String::createFromUtf8(runtime, "androidSetVolume");
      case Name::ANDROID_SET_RATING:
        return jsi::String::createFromUtf8(runtime, "androidSetRating");
      case Name::IOS_TOGGLE_PLAY_PAUSE:
        return jsi::String::createFromUtf8(runtime, "iosTogglePlayPause");
      case Name::IOS_ENABLE_LANGUAGE_OPTIONS:
        return jsi::String::createFromUtf8(runtime, "iosEnableLanguageOptions");
      case Name::IOS_DISABLE_LANGUAGE_OPTIONS:
        return jsi::String::createFromUtf8(runtime, "iosDisableLanguageOptions");
    }
  }

 private:
  Name name_;
};

} // namespace audioapi
