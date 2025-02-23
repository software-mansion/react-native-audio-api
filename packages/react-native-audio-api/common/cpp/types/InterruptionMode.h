#pragma once

#include <jsi/jsi.h>
#include <string>

namespace audioapi {
using namespace facebook;

class InterruptionMode {
 public:
  enum IMode {
    MANUAL,
    DISABLED,
    AUTOMATIC,
  };

  InterruptionMode() = default;
  explicit constexpr InterruptionMode(IMode mode) : mode_(mode) {}

  constexpr operator IMode() const { return mode_; }
  explicit operator bool() const = delete;
  constexpr bool operator==(InterruptionMode other) const { return mode_ == other.mode_; }
  constexpr bool operator!=(InterruptionMode other) const { return mode_ != other.mode_; }

  static InterruptionMode fromJSIValue(const jsi::Value &value, jsi::Runtime &runtime) {
    std::string modeName = value.asString(runtime).utf8(runtime);

    if (modeName == "manual") {
      return InterruptionMode(InterruptionMode::MANUAL);
    }

    if (modeName == "disabled") {
      return InterruptionMode(InterruptionMode::DISABLED);
    }

    if (modeName == "automatic") {
      return InterruptionMode(InterruptionMode::AUTOMATIC);
    }

    return InterruptionMode(InterruptionMode::AUTOMATIC);
  }

  jsi::Value toJSIValue(jsi::Runtime &runtime) const {
    switch (mode_) {
      case InterruptionMode::MANUAL:
        return jsi::String::createFromUtf8(runtime, "manual");
      case InterruptionMode::DISABLED:
        return jsi::String::createFromUtf8(runtime, "disabled");
      case InterruptionMode::AUTOMATIC:
        return jsi::String::createFromUtf8(runtime, "automatic");
    }
  }

 private:
  IMode mode_;
};

} // namespace audioapi
