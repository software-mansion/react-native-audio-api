#pragma once

#include <jsi/jsi.h>
#include <vector>
#include <memory>

#include "IOSMode.h"
#include "IOSCategory.h"
#include "InterruptionMode.h"
#include "IOSCategoryOption.h"

namespace audioapi {
using namespace facebook;

class AudioSessionOptions {
 public:
  static std::shared_ptr<AudioSessionOptions> fromJSIValue(const jsi::Value &jsiValue, jsi::Runtime &runtime) {
    auto jsiOptionsObject = jsiValue.getObject(runtime);
    auto options = std::make_shared<AudioSessionOptions>();

    // Handle enum properties
    options->iosMode = IOSMode::fromJSIValue(jsiOptionsObject.getProperty(runtime, "iosMode"), runtime);
    options->iosCategory = IOSCategory::fromJSIValue(jsiOptionsObject.getProperty(runtime, "iosCategory"), runtime);
    options->interruptionMode = InterruptionMode::fromJSIValue(jsiOptionsObject.getProperty(runtime, "interruptionMode"), runtime);

    options->androidForegroundService = jsiOptionsObject.getProperty(runtime, "androidForegroundService").getBool();

    // Handle array properties
    auto iosCategoryOptionsArray = jsiOptionsObject.getProperty(runtime, "iosCategoryOptions").getObject(runtime).getArray(runtime);
    size_t iosCategoryOptionsSize = iosCategoryOptionsArray.size(runtime);

    for (size_t i = 0; i < iosCategoryOptionsSize; i += 1) {
      options->iosCategoryOptions.push_back(IOSCategoryOption::fromJSIValue(iosCategoryOptionsArray.getValueAtIndex(runtime, i), runtime));
    }

    return options;
  }

  jsi::Value toJSIValue(jsi::Runtime &runtime) const {
    auto jsiOptions = jsi::Object(runtime);

    // Handle enum properties
    jsiOptions.setProperty(runtime, "iosMode", iosMode.toJSIValue(runtime));
    jsiOptions.setProperty(runtime, "iosCategory", iosCategory.toJSIValue(runtime));
    jsiOptions.setProperty(runtime, "interruptionMode", interruptionMode.toJSIValue(runtime));

    jsiOptions.setProperty(runtime, "androidForegroundService", jsi::Value(androidForegroundService));

    // Handle array properties
    auto iosCategoryOptionsArray = jsi::Array(runtime, iosCategoryOptions.size());

    for (size_t i = 0; i < iosCategoryOptions.size(); i += 1) {
      iosCategoryOptionsArray.setValueAtIndex(runtime, i, iosCategoryOptions[i].toJSIValue(runtime));
    }

    jsiOptions.setProperty(runtime, "iosCategoryOptions", iosCategoryOptionsArray);

    return jsiOptions;
  }

  IOSMode iosMode;
  IOSCategory iosCategory;
  bool androidForegroundService;
  InterruptionMode interruptionMode;
  std::vector<IOSCategoryOption> iosCategoryOptions;
};

} // namespace audioapi

