#pragma once

#include <audioworklets/core/WorkletNodeDomain.h>

#include <jsi/jsi.h>

namespace audioworklets::option_parser {

using namespace facebook;

inline WorkletNodeDomain parseWorkletNodeDomain(jsi::Runtime &runtime, const jsi::Value &value) {
  if (!value.isString()) {
    throw jsi::JSError(
        runtime,
        "[react-native-audio-worklets] domain must be a string ('time-domain' or 'frequency-domain')");
  }

  const auto domainStr = value.asString(runtime).utf8(runtime);
  if (domainStr == "time-domain") {
    return WorkletNodeDomain::TimeDomain;
  }
  if (domainStr == "frequency-domain") {
    return WorkletNodeDomain::FrequencyDomain;
  }

  throw jsi::JSError(
      runtime, "[react-native-audio-worklets] domain must be 'time-domain' or 'frequency-domain'");
}

} // namespace audioworklets::option_parser
