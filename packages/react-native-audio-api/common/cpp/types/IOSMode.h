#pragma once

#include <jsi/jsi.h>
#include <vector>
#include <memory>
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

  static IOSMode fromJSIValue(const jsi::Value &value, jsi::Runtime &runtime) {
    std::string modeName = value.asString(runtime).utf8(runtime);

    if (modeName == "default") {
      return IOSMode(Mode::DEFAULT);
    }

    if (modeName == "gameChat") {
      return IOSMode(Mode::GAME_CHAT);
    }

    if (modeName == "videoChat") {
      return IOSMode(Mode::VIDEO_CHAT);
    }

    if (modeName == "voiceChat") {
      return IOSMode(Mode::VOICE_CHAT);
    }

    if (modeName == "measurement") {
      return IOSMode(Mode::MEASUREMENT);
    }

    if (modeName == "voicePrompt") {
      return IOSMode(Mode::VOICE_PROMPT);
    }

    if (modeName == "spokenAudio") {
      return IOSMode(Mode::SPOKEN_AUDIO);
    }

    if (modeName == "moviePlayback") {
      return IOSMode(Mode::MOVIE_PLAYBACK);
    }

    if (modeName == "videoRecording") {
      return IOSMode(Mode::VIDEO_RECORDING);
    }

    return IOSMode(Mode::DEFAULT);
  }

  jsi::Value toJSIValue(jsi::Runtime &runtime) const {
    switch (mode_) {
      case Mode::DEFAULT:
        return jsi::String::createFromUtf8(runtime, "default");
      case Mode::GAME_CHAT:
        return jsi::String::createFromUtf8(runtime, "gameChat");
      case Mode::VIDEO_CHAT:
        return jsi::String::createFromUtf8(runtime, "videoChat");
      case Mode::VOICE_CHAT:
        return jsi::String::createFromUtf8(runtime, "voiceChat");
      case Mode::MEASUREMENT:
        return jsi::String::createFromUtf8(runtime, "measurement");
      case Mode::VOICE_PROMPT:
        return jsi::String::createFromUtf8(runtime, "voicePrompt");
      case Mode::SPOKEN_AUDIO:
        return jsi::String::createFromUtf8(runtime, "spokenAudio");
      case Mode::MOVIE_PLAYBACK:
        return jsi::String::createFromUtf8(runtime, "moviePlayback");
      case Mode::VIDEO_RECORDING:
        return jsi::String::createFromUtf8(runtime, "videoRecording");
    }
  }

 private:
  Mode mode_;
};

} // namespace audioapi
