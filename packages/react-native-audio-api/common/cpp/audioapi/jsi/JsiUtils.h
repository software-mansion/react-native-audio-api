#pragma once

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

} // namespace audioapi::jsiutils
