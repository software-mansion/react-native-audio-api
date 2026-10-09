#pragma once

#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/core/types/AudioContextLatencyHint.h>
#include <audioapi/core/types/AudioContextOptions.h>
#include <audioapi/core/types/BiquadFilterType.h>
#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/types/ContextState.h>
#include <audioapi/core/types/OscillatorType.h>
#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/core/types/PannerTypes.h>
#include <audioapi/events/AudioEvent.h>
#include <string>

namespace audioapi::js_enum_parser {

std::string overSampleTypeToString(OverSampleType type);
OverSampleType overSampleTypeFromString(const std::string &type);
std::string oscillatorTypeToString(OscillatorType type);
OscillatorType oscillatorTypeFromString(const std::string &type);
std::string filterTypeToString(BiquadFilterType type);
BiquadFilterType filterTypeFromString(const std::string &type);
AudioEvent audioEventFromString(const std::string &event);
std::string channelCountModeToString(ChannelCountMode mode);
ChannelCountMode channelCountModeFromString(const std::string &mode);
std::string channelInterpretationToString(ChannelInterpretation interpretation);
std::string panningModelToString(PanningModelType model);
PanningModelType panningModelFromString(const std::string &model);
std::string distanceModelToString(DistanceModelType model);
DistanceModelType distanceModelFromString(const std::string &model);
ChannelInterpretation channelInterpretationFromString(const std::string &interpretation);
std::string contextStateToString(ContextState state);
/// Interactive, the spec default, for an unrecognised string; a browser would throw a TypeError.
AudioContextLatencyHint latencyHintFromString(const std::string &hint);
/// Media, the default profile, for an unrecognised string.
AndroidOutputProfile androidOutputProfileFromString(const std::string &profile);
} // namespace audioapi::js_enum_parser
