#pragma once

#include <jsi/jsi.h>
#include <string>

namespace audioapi {
using namespace facebook;

class IOSMode {
 public:
  enum Mode {
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

  IOSMode() = default;
  explicit constexpr IOSMode(Mode mode) : mode_(mode) {}

  constexpr operator Mode() const { return mode_; }
  explicit operator bool() const = delete;
  constexpr bool operator==(IOSMode other) const { return mode_ == other.mode_; }
  constexpr bool operator!=(IOSMode other) const { return mode_ != other.mode_; }

  static Mode fromString(const std::string &modeName) {
    if (modeName == "default") {
      return Mode::DEFAULT;
    }

    if (modeName == "gameChat") {
      return Mode::GAME_CHAT;
    }

    if (modeName == "videoChat") {
      return Mode::VIDEO_CHAT;
    }

    if (modeName == "voiceChat") {
      return Mode::VOICE_CHAT;
    }

    if (modeName == "measurement") {
      return Mode::MEASUREMENT;
    }

    if (modeName == "voicePrompt") {
      return Mode::VOICE_PROMPT;
    }

    if (modeName == "spokenAudio") {
      return Mode::SPOKEN_AUDIO;
    }

    if (modeName == "moviePlayback") {
      return Mode::MOVIE_PLAYBACK;
    }

    if (modeName == "videoRecording") {
      return Mode::VIDEO_RECORDING;
    }

    return Mode::DEFAULT;
  }

  static IOSMode fromJSIValue(const jsi::Value &value, jsi::Runtime &runtime) {
    std::string modeName = value.asString(runtime).utf8(runtime);
    return IOSMode(IOSMode::fromString(modeName));
  }

  Mode value() const {
    return mode_;
  }

  std::string strValue() const {
    switch (mode_) {
      case Mode::DEFAULT:
        return "default";
      case Mode::GAME_CHAT:
        return "gameChat";
      case Mode::VIDEO_CHAT:
        return "videoChat";
      case Mode::VOICE_CHAT:
        return "voiceChat";
      case Mode::MEASUREMENT:
        return "measurement";
      case Mode::VOICE_PROMPT:
        return "voicePrompt";
      case Mode::SPOKEN_AUDIO:
        return "spokenAudio";
      case Mode::MOVIE_PLAYBACK:
        return "moviePlayback";
      case Mode::VIDEO_RECORDING:
        return "videoRecording";
      default:
        return "default";
    }
  }

  jsi::Value toJSIValue(jsi::Runtime &runtime) const {
    return jsi::String::createFromUtf8(runtime, strValue());
  }

 private:
  Mode mode_;
};

} // namespace audioapi
