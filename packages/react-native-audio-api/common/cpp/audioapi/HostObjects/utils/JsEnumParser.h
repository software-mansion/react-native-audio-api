#pragma once

#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/core/types/BiquadFilterType.h>
#include <audioapi/core/types/OscillatorType.h>
#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/events/AudioEvent.h>
#include <string>

namespace audioapi::js_enum_parser {
std::string windowTypeToString(AnalyserNode::WindowType type);
AnalyserNode::WindowType windowTypeFromString(const std::string &type);
std::string overSampleTypeToString(OverSampleType type);
OverSampleType overSampleTypeFromString(const std::string &type);
std::string oscillatorTypeToString(OscillatorType type);
OscillatorType oscillatorTypeFromString(const std::string &type);
std::string filterTypeToString(BiquadFilterType type);
BiquadFilterType filterTypeFromString(const std::string &type);
AudioEvent audioEventFromString(const std::string &event);
} // namespace audioapi::js_enum_parser
