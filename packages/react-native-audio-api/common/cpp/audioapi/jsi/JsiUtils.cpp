#include <audioapi/jsi/JsiUtils.h>
#include <string>

namespace audioapi::jsiutils {

using namespace facebook;

std::string argToString(
    jsi::Runtime &runtime,
    const jsi::Value *args,
    size_t count,
    size_t index,
    const std::string &defaultValue) {
  if (index < count && args[index].isString()) {
    return args[index].asString(runtime).utf8(runtime);
  }

  return defaultValue;
}

} // namespace audioapi::jsiutils
