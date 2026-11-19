#include <audioapi/jsi/JsiUtils.h>
#include <string>
#include <utility>

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

[[noreturn]] void
throwException(jsi::Runtime &runtime, const char *name, const std::string &message) {
  auto errorConstructor = runtime.global().getPropertyAsFunction(runtime, "Error");
  auto error =
      errorConstructor.callAsConstructor(runtime, jsi::String::createFromUtf8(runtime, message));
  auto errorObject = error.asObject(runtime);
  errorObject.setProperty(runtime, "name", jsi::String::createFromUtf8(runtime, name));
  throw jsi::JSError(runtime, std::move(error));
}

jsi::Object toJsResult(jsi::Runtime &runtime, const Result<NoneType, std::string> &result) {
  auto jsResult = jsi::Object(runtime);
  jsResult.setProperty(
      runtime,
      "status",
      jsi::String::createFromUtf8(runtime, result.is_ok() ? "success" : "error"));
  if (result.is_err()) {
    jsResult.setProperty(
        runtime, "message", jsi::String::createFromUtf8(runtime, result.unwrap_err()));
  }
  return jsResult;
}

} // namespace audioapi::jsiutils
