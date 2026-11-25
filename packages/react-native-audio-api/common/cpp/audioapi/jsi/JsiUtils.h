#pragma once

#include <audioapi/utils/Result.hpp>

#include <jsi/jsi.h>
#include <string>

namespace audioapi::jsiutils {

using namespace facebook;

std::string argToString(
    jsi::Runtime &runtime,
    const jsi::Value *args,
    size_t count,
    size_t index,
    const std::string &defaultValue = "");

[[noreturn]] void
throwException(jsi::Runtime &runtime, const char *name, const std::string &message);

/// The `Result` shape the TS layer reads: `{status: 'success'}` or `{status: 'error', message}`.
jsi::Object toJsResult(jsi::Runtime &runtime, const Result<NoneType, std::string> &result);

} // namespace audioapi::jsiutils
