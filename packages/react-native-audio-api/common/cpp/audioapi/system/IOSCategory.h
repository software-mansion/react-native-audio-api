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

  static Category fromString(const std::string &categoryName) {
    if (categoryName == "record") {
      return Category::RECORD;
    }

    if (categoryName == "ambient") {
      return Category::AMBIENT;
    }

    if (categoryName == "playback") {
      return Category::PLAYBACK;
    }

    if (categoryName == "multiRoute") {
      return Category::MULTI_ROUTE;
    }

    if (categoryName == "soloAmbient") {
      return Category::SOLO_AMBIENT;
    }

    if (categoryName == "playbackAndRecord") {
      return Category::PLAYBACK_AND_RECORD;
    }

    return Category::PLAYBACK;
  }

  static IOSCategory fromJSIValue(const jsi::Value &value, jsi::Runtime &runtime) {
    std::string categoryName = value.asString(runtime).utf8(runtime);
    return IOSCategory(fromString(categoryName));
  }

  Category value() const {
    return category_;
  }

  std::string strValue() const {
    switch (category_) {
      case Category::RECORD:
        return "record";
      case Category::AMBIENT:
        return "ambient";
      case Category::PLAYBACK:
        return "playback";
      case Category::MULTI_ROUTE:
        return "multiRoute";
      case Category::SOLO_AMBIENT:
        return "soloAmbient";
      case Category::PLAYBACK_AND_RECORD:
        return "playbackAndRecord";
      default:
        return "playback";
    }
  }

  jsi::Value toJSIValue(jsi::Runtime &runtime) const {
    return jsi::String::createFromUtf8(runtime, strValue());
  }

 private:
  Category category_;
};

} // namespace audioapi
