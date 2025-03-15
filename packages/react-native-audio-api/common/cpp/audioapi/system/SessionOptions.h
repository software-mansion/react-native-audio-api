#pragma once

#include <audioapi/system/IOSMode.h>
#include <audioapi/system/IOSCategory.h>
#include <audioapi/system/IOSCategoryOption.h>

#include <jsi/jsi.h>
#include <vector>
#include <memory>

namespace audioapi {
using namespace facebook;

class SessionOptions {
 public:
  static std::shared_ptr<SessionOptions> fromJSIValue(
    const jsi::Value &jsiValue,
    jsi::Runtime &runtime) {
    auto jsiOptionsObject = jsiValue.getObject(runtime);
    auto options = std::make_shared<SessionOptions>();

    options->iosCategory = IOSCategory::fromJSIValue(
      jsiOptionsObject.getProperty(runtime, "iosCategory"),
      runtime
    );

    options->iosMode = IOSMode::fromJSIValue(
      jsiOptionsObject.getProperty(runtime, "iosMode"),
      runtime
    );

    auto optionsArray = jsiOptionsObject
      .getProperty(runtime, "iosOptions")
      .getObject(runtime)
      .getArray(runtime);

    size_t optionsArraySize = optionsArray.size(runtime);

    for (size_t i = 0; i < optionsArraySize; i++) {
      options->iosCategoryOptions.push_back(
        IOSCategoryOption::fromJSIValue(
          optionsArray.getValueAtIndex(runtime, i),
          runtime
        )
      );
    }

    return options;
  }

  jsi::Value toJSIValue(jsi::Runtime &runtime) const {
    auto jsiValue = jsi::Object(runtime);

    jsiValue.setProperty(runtime, "iosCategory", iosCategory.toJSIValue(runtime));
    jsiValue.setProperty(runtime, "iosMode", iosMode.toJSIValue(runtime));

    auto optionsArray = jsi::Array(runtime, iosCategoryOptions.size());

    for (size_t i = 0; i < iosCategoryOptions.size(); i++) {
      optionsArray.setValueAtIndex(runtime, i, iosCategoryOptions[i].toJSIValue(runtime));
    }

    jsiValue.setProperty(runtime, "iosOptions", optionsArray);

    return jsiValue;
  }

  IOSMode iosMode;
  IOSCategory iosCategory;
  std::vector<IOSCategoryOption> iosCategoryOptions;
};

} // namespace audioapi

