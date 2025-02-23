#pragma once

#include <jsi/jsi.h>
#include <string>

namespace audioapi {
using namespace facebook;

class IOSCategory {
 public:
  enum Category {
    RECORD,
    AMBIENT,
    PLAYBACK,
    MULTI_ROUTE,
    SOLO_AMBIENT,
    PLAYBACK_AND_RECORD,
  };

  IOSCategory() = default;
  explicit constexpr IOSCategory(Category category) : category_(category) {}

  constexpr operator Category() const { return category_; }
  explicit operator bool() const = delete;
  constexpr bool operator==(IOSCategory other) const { return category_ == other.category_; }
  constexpr bool operator!=(IOSCategory other) const { return category_ != other.category_; }

  static IOSCategory fromJSIValue(const jsi::Value &value, jsi::Runtime &runtime) {
    std::string categoryName = value.asString(runtime).utf8(runtime);

    if (categoryName == "record") {
      return IOSCategory(Category::RECORD);
    }

    if (categoryName == "ambient") {
      return IOSCategory(Category::AMBIENT);
    }

    if (categoryName == "playback") {
      return IOSCategory(Category::PLAYBACK);
    }

    if (categoryName == "multiRoute") {
      return IOSCategory(Category::MULTI_ROUTE);
    }

    if (categoryName == "soloAmbient") {
      return IOSCategory(Category::SOLO_AMBIENT);
    }

    if (categoryName == "playbackAndRecord") {
      return IOSCategory(Category::PLAYBACK_AND_RECORD);
    }

    return IOSCategory(Category::PLAYBACK);
  }

  jsi::Value toJSIValue(jsi::Runtime &runtime) const {
    switch (category_) {
      case Category::RECORD:
        return jsi::String::createFromUtf8(runtime, "record");
      case Category::AMBIENT:
        return jsi::String::createFromUtf8(runtime, "ambient");
      case Category::PLAYBACK:
        return jsi::String::createFromUtf8(runtime, "playback");
      case Category::MULTI_ROUTE:
        return jsi::String::createFromUtf8(runtime, "multiRoute");
      case Category::SOLO_AMBIENT:
        return jsi::String::createFromUtf8(runtime, "soloAmbient");
      case Category::PLAYBACK_AND_RECORD:
        return jsi::String::createFromUtf8(runtime, "playbackAndRecord");
    }
  }

 private:
  Category category_;
};

} // namespace audioapi
